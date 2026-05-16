"""
GenerateCode.py — Unreal-style header tool for KraftonEngine.

Scans Source/ for headers containing UCLASS() and emits per-class:
    Intermediate/Generated/Inc/<File>.generated.h
    Intermediate/Generated/Source/<File>.gen.cpp

Usage:
    python Scripts/GenerateCode.py [--clean] [--verbose]
"""

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path


# ──────────────────────────────────────────────
# Paths
# ──────────────────────────────────────────────
ROOT = Path(__file__).resolve().parent.parent
SOURCE_ROOTS = [
    ROOT / "KraftonEngine" / "Source" / "Engine",
    ROOT / "KraftonEngine" / "Source" / "Editor",
    ROOT / "KraftonEngine" / "Source" / "ObjViewer",
    ROOT / "KraftonEngine" / "Source" / "Game",
]
OUT_INC = ROOT / "KraftonEngine" / "Intermediate" / "Generated" / "Inc"
OUT_SRC = ROOT / "KraftonEngine" / "Intermediate" / "Generated" / "Source"

# Match include-root order from CLAUDE.md so generated #include paths
# resolve under the same roots existing code uses.
INCLUDE_ROOTS = [
    ROOT / "KraftonEngine" / "Source" / "Engine",
    ROOT / "KraftonEngine" / "Source",
    ROOT / "KraftonEngine" / "Source" / "Editor",
    ROOT / "KraftonEngine" / "Source" / "ObjViewer",
]


class CodegenError(Exception):
    pass


# ──────────────────────────────────────────────
# Source Discovery
# ──────────────────────────────────────────────
def discover_headers() -> list[Path]:
    headers = []
    for root in SOURCE_ROOTS:
        if not root.exists():
            continue
        for path in root.rglob("*.h"):
            text = path.read_text(encoding="utf-8-sig")  # tolerate BOM
            if any(marker in text for marker in ("UCLASS(", "UENUM(", "USTRUCT(")):
                headers.append(path)
    return headers


# ──────────────────────────────────────────────
# Comment Stripping
# ──────────────────────────────────────────────
_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
_LINE_COMMENT  = re.compile(r"//[^\n]*")

def strip_comments(src: str) -> str:
    src = _BLOCK_COMMENT.sub("", src)
    src = _LINE_COMMENT.sub("", src)
    return src


# ──────────────────────────────────────────────
# Parse Model
# ──────────────────────────────────────────────
@dataclass
class PropertyInfo:
    name: str                  # member name (C++ identifier)
    cpp_type: str              # raw C++ type as written
    prop_type: str             # EPropertyType::Float etc
    category: str = ""
    display_name: str | None = None  # DisplayName="..." — falls back to name
    flags: list[str] = field(default_factory=lambda: ["CPF_None"])
    min: str | None = None     # raw expression, emitted as-is
    max: str | None = None
    speed: str | None = None
    enum_names: str | None = None
    enum_count: str | None = None
    enum_size: str | None = None
    enum_type: str | None = None
    struct_func: str | None = None
    array_inner_type: str | None = None  # for TArray<T>


@dataclass
class FunctionInfo:
    name: str
    flags: list[str] = field(default_factory=list)
    lua_name: str | None = None   # LuaName="..." override

    @property
    def is_lua(self) -> bool:
        return "Lua" in self.flags


@dataclass
class ClassInfo:
    name: str                # ex: APlayerController
    parent: str              # ex: APawn
    class_flags: list[str] = field(default_factory=lambda: ["CF_None"])
    properties: list[PropertyInfo] = field(default_factory=list)
    functions: list[FunctionInfo] = field(default_factory=list)
    no_factory: bool = False  # UCLASS(NoFactory) — skip FObjectFactory registration

@dataclass
class EnumInfo:
    name: str
    entries: list[str]
    underlying_type: str | None = None

@dataclass
class HeaderInfo:
    classes: list[ClassInfo] = field(default_factory=list)
    enums: list[EnumInfo] = field(default_factory=list)


# ──────────────────────────────────────────────
# Regex Patterns
# ──────────────────────────────────────────────
# Annotation argument lists may contain quoted strings with ')' characters,
# e.g. DisplayName="Amplitude (deg)". Keep the regex parser small, but do not
# terminate an annotation while still inside a quoted string.
ANNOTATION_ARGS_RE = r'((?:[^)"\n]|"[^"]*")*)'

# UCLASS(...) followed by class declaration. Captures: flags, class name, parent.
CLASS_RE = re.compile(
    rf"UCLASS\s*\({ANNOTATION_ARGS_RE}\)\s*"
    r"class\s+(?:\w+\s+)?(\w+)\s*"          # optional API-export tag
    r":\s*public\s+(\w+)",                  # single public base
    re.MULTILINE,
)

ENUM_RE = re.compile(
    rf"UENUM\s*\({ANNOTATION_ARGS_RE}\)\s*"
    r"enum\s+class\s+(\w+)"
    r"(?:\s*:\s*(\w+))?",
    re.MULTILINE,
)

# UPROPERTY(...) <decl-up-to-semicolon> ;
PROPERTY_RE = re.compile(
    rf"UPROPERTY\s*\({ANNOTATION_ARGS_RE}\)\s*([^;]+);",
    re.MULTILINE,
)

# UFUNCTION(...) <return-type> <name> ( ... ) ;
FUNCTION_RE = re.compile(
    rf"UFUNCTION\s*\({ANNOTATION_ARGS_RE}\)\s*"
    r"[\w\s\*&:<>,]+?\s+"                   # return type (greedy-ish)
    r"(\w+)\s*\([^)]*\)\s*[^;{]*[;{]",
    re.MULTILINE,
)


# ──────────────────────────────────────────────
# Attribute Parser
# ──────────────────────────────────────────────
_KV_RE = re.compile(r'(\w+)\s*=\s*("[^"]*"|[^,]+)')

def parse_attributes(attr_text: str) -> tuple[list[str], dict[str, str]]:
    """Returns (flags, key_value_map). Strings keep their quotes stripped."""
    flags: list[str] = []
    kvs: dict[str, str] = {}

    def take_kv(m):
        key, val = m.group(1), m.group(2).strip()
        if val.startswith('"') and val.endswith('"'):
            val = val[1:-1]
        kvs[key] = val
        return ""

    remainder = _KV_RE.sub(take_kv, attr_text)

    for token in remainder.split(","):
        token = token.strip()
        if token:
            flags.append(token)
    return flags, kvs


PROPERTY_FLAG_MAP = {
    "None":      "CPF_None",
    "Edit":      "CPF_Edit",
    "Transient": "CPF_Transient",
    "Config":    "CPF_Config",
    "FixedSize": "CPF_FixedSize",
}

CLASS_FLAG_MAP = {
    "Actor":                 "CF_Actor",
    "Component":             "CF_Component",
    "Camera":                "CF_Camera",
    "HiddenInComponentList": "CF_HiddenInComponentList",
}

# UCLASS attributes consumed by codegen itself (not forwarded to CF_*).
CLASS_META_ATTRS = {"NoFactory"}


# ──────────────────────────────────────────────
# Type Mapping
# ──────────────────────────────────────────────
TYPE_MAP = {
    "bool":            ("EPropertyType::Bool",     "PROPERTY_BOOL"),
    "int":             ("EPropertyType::Int",      "PROPERTY_INT"),
    "int32":           ("EPropertyType::Int",      "PROPERTY_INT"),
    "uint32":          ("EPropertyType::Int",      "PROPERTY_INT"),
    "float":           ("EPropertyType::Float",    "PROPERTY_FLOAT"),
    "FVector":         ("EPropertyType::Vec3",     "PROPERTY_VEC3"),
    "FVector4":        ("EPropertyType::Vec4",     None),
    "FRotator":        ("EPropertyType::Rotator",  None),
    "FString":         ("EPropertyType::String",   "PROPERTY_STRING"),
    "std::string":     ("EPropertyType::String",   "PROPERTY_STRING"),
    "FName":           ("EPropertyType::Name",     None),
    "FMaterialSlot":   ("EPropertyType::MaterialSlot", None),
}

# uint8 → ByteBool requires explicit Type=ByteBool override since uint8 is
# ambiguous with small-int usage. Default to Int if no override.
TYPE_MAP["uint8"] = ("EPropertyType::Int", "PROPERTY_INT")

POINTER_TYPE_MAP = {
    "UStaticMesh":     "EPropertyType::StaticMeshRef",
    "USkeletalMesh":   "EPropertyType::SkeletalMeshRef",
    "USceneComponent": "EPropertyType::SceneComponentRef",
}


def classify_type(
    cpp_type: str,
    known_enums: dict[str, EnumInfo],
) -> tuple[str, str | None, str | None]:
    """Returns (EPropertyType, helper_macro_or_None, array_inner_type_or_None)."""
    t = cpp_type.strip()

    if t.endswith("*"):
        base = t.rstrip("*").strip()
        et = POINTER_TYPE_MAP.get(base)
        if et:
            return et, None, None
        raise CodegenError(
            f"unknown pointer type '{t}' — add to POINTER_TYPE_MAP or use UPROPERTY(Type=...)"
        )

    if t.startswith("TArray<") and t.endswith(">"):
        inner = t[len("TArray<"):-1].strip()
        return "EPropertyType::Array", None, inner

    if t in TYPE_MAP:
        et, helper = TYPE_MAP[t]
        return et, helper, None
    
    if t in known_enums:
        return "EPropertyType::Enum", None, None

    raise CodegenError(
        f"unknown type '{t}' — add to TYPE_MAP, mark as UENUM/USTRUCT, or use UPROPERTY(Type=...)"
    )


# ──────────────────────────────────────────────
# Header Parser
# ──────────────────────────────────────────────
def find_braced_body(src: str, start_idx: int) -> str:
    """Given an offset just after a declaration, return text between
    the matching {...}. Handles nested braces (inner struct/method bodies)."""
    i = src.find("{", start_idx)
    if i == -1:
        return ""
    depth = 1
    body_start = i + 1
    i += 1
    while i < len(src) and depth > 0:
        ch = src[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return src[body_start:i]
        i += 1
    return src[body_start:]   # unterminated — return what we have


def parse_enum_entries(body: str) -> list[str]:
    entries: list[str] = []
    for raw_entry in body.split(","):
        entry = raw_entry.split("=", 1)[0].strip()
        if not entry:
            continue
        if not re.fullmatch(r"\w+", entry):
            raise CodegenError(f"cannot parse enum entry: {raw_entry.strip()!r}")
        entries.append(entry)
    return entries


def parse_enums(path: Path) -> list[EnumInfo]:
    text = strip_comments(path.read_text(encoding="utf-8-sig"))
    enums: list[EnumInfo] = []

    for m in ENUM_RE.finditer(text):
        _, name, underlying_type = m.group(1), m.group(2), m.group(3)
        body = find_braced_body(text, m.end())
        if not body:
            raise CodegenError(f"{path}: could not locate enum body for {name}")
        entries = parse_enum_entries(body)
        if not entries:
            raise CodegenError(f"{path}: enum {name} has no entries")
        enums.append(EnumInfo(name=name, entries=entries, underlying_type=underlying_type))
    return enums


def build_enum_registry(headers: list[Path]) -> dict[str, EnumInfo]:
    registry: dict[str, EnumInfo] = {}
    owners: dict[str, Path] = {}
    for path in headers:
        for enum in parse_enums(path):
            if enum.name in registry:
                raise CodegenError(
                    f"duplicate UENUM {enum.name}: {path} and {owners[enum.name]}"
                )
            registry[enum.name] = enum
            owners[enum.name] = path
    return registry


def parse_property(
    attr_text: str,
    decl_text: str,
    known_enums: dict[str, EnumInfo],
) -> PropertyInfo:
    flags_raw, kvs = parse_attributes(attr_text)

    # Strip default initializer ("= 50.f")
    if "=" in decl_text:
        decl_text = decl_text.split("=", 1)[0]
    decl_text = decl_text.strip()

    # Last identifier = member name, everything before = type.
    m = re.match(r"(.+?)\s+(\w+)\s*$", decl_text)
    if not m:
        raise CodegenError(f"cannot parse property declaration: {decl_text!r}")
    cpp_type, name = m.group(1).strip(), m.group(2)
    enum_type = None

    # Explicit Type= override bypasses classify_type.
    if "Type" in kvs:
        prop_type = f"EPropertyType::{kvs['Type']}"
        array_inner = None
    else:
        prop_type, _, array_inner = classify_type(cpp_type, known_enums)
        if cpp_type in known_enums:
            enum_type = cpp_type

    flags = [PROPERTY_FLAG_MAP.get(f, f"CPF_{f}") for f in flags_raw] or ["CPF_None"]

    return PropertyInfo(
        name=name,
        cpp_type=cpp_type,
        prop_type=prop_type,
        category=kvs.get("Category", ""),
        display_name=kvs.get("DisplayName"),
        flags=flags,
        min=kvs.get("Min"),
        max=kvs.get("Max"),
        speed=kvs.get("Speed"),
        enum_names=kvs.get("EnumNames"),
        enum_count=kvs.get("EnumCount"),
        enum_size=kvs.get("EnumSize"),
        enum_type=enum_type,
        struct_func=kvs.get("StructFunc"),
        array_inner_type=array_inner,
    )


def parse_function(attr_text: str, name: str) -> FunctionInfo:
    flags_raw, kvs = parse_attributes(attr_text)
    return FunctionInfo(name=name, flags=flags_raw, lua_name=kvs.get("LuaName"))


def parse_header(path: Path, known_enums: dict[str, EnumInfo]) -> list[ClassInfo]:
    text = strip_comments(path.read_text(encoding="utf-8-sig"))
    classes: list[ClassInfo] = []

    for m in CLASS_RE.finditer(text):
        attr_text, name, parent = m.group(1), m.group(2), m.group(3)
        flags_raw, _ = parse_attributes(attr_text)

        # Split codegen-meta attrs (NoFactory) from runtime class flags (CF_*).
        meta = {f for f in flags_raw if f in CLASS_META_ATTRS}
        runtime_flags_raw = [f for f in flags_raw if f not in CLASS_META_ATTRS]
        class_flags = (
            [CLASS_FLAG_MAP.get(f, f"CF_{f}") for f in runtime_flags_raw] or ["CF_None"]
        )

        body = find_braced_body(text, m.end())
        if not body:
            raise CodegenError(f"{path}: could not locate class body for {name}")

        properties = [
            parse_property(pm.group(1), pm.group(2), known_enums)
            for pm in PROPERTY_RE.finditer(body)
        ]
        functions = [
            parse_function(fm.group(1), fm.group(2))
            for fm in FUNCTION_RE.finditer(body)
        ]

        classes.append(ClassInfo(
            name=name,
            parent=parent,
            class_flags=class_flags,
            properties=properties,
            functions=functions,
            no_factory="NoFactory" in meta,
        ))
    return classes


# ──────────────────────────────────────────────
# Emission — .generated.h
# ──────────────────────────────────────────────
GENERATED_H_TEMPLATE = """\
// AUTOGENERATED by GenerateCode.py — do not edit.
#pragma once

{class_macros}
"""

CLASS_MACRO_TEMPLATE = """\
#define KE_GENERATED_BODY_{class_name}() \\
    using Super = {parent}; \\
    static UClass StaticClassInstance; \\
    static FClassRegistrar s_Registrar; \\
    static UClass* StaticClass() {{ return &StaticClassInstance; }} \\
    UClass* GetClass() const override {{ return StaticClass(); }} \\
    friend struct {class_name}_PropertyRegistrar;
"""

def emit_generated_header(classes: list[ClassInfo]) -> str:
    macros = "\n".join(
        CLASS_MACRO_TEMPLATE.format(class_name=c.name, parent=c.parent)
        for c in classes
    )
    return GENERATED_H_TEMPLATE.format(class_macros=macros)


# ──────────────────────────────────────────────
# Emission — .gen.cpp
# ──────────────────────────────────────────────
def emit_gen_cpp(
    source_header_include: str,
    classes: list[ClassInfo],
    known_enums: dict[str, EnumInfo],
) -> str:
    has_lua = any(fn.is_lua for c in classes for fn in c.functions)
    out = [
        "// AUTOGENERATED by GenerateCode.py — do not edit.",
        f'#include "{source_header_include}"',
        '#include "Object/ObjectFactory.h"',
    ]
    if has_lua:
        out.append('#include "Object/LuaClassRegistry.h"')
        out.append("#include <sol/sol.hpp>")
    out.append("")
    for c in classes:
        out.append(emit_class_static(c))
        out.append(emit_property_registrar(c, known_enums))
        lua_block = emit_lua_registrar(c)
        if lua_block:
            out.append(lua_block)
    return "\n".join(out)


def emit_class_static(c: ClassInfo) -> str:
    flags = " | ".join(c.class_flags)
    parts = [
        f"UClass {c.name}::StaticClassInstance(",
        f'    "{c.name}", &{c.parent}::StaticClassInstance,',
        f"    sizeof({c.name}), {flags});",
        f"FClassRegistrar {c.name}::s_Registrar(&{c.name}::StaticClassInstance);",
    ]
    # Default: register with FObjectFactory so SceneSaveManager can spawn
    # this type by name (matches IMPLEMENT_CLASS = DEFINE_CLASS + REGISTER_FACTORY
    # used by 88/90 UObject-derived classes in the engine today).
    if not c.no_factory:
        parts.append(f"REGISTER_FACTORY({c.name})")
    return "\n".join(parts) + "\n"


def emit_property_registrar(c: ClassInfo, known_enums: dict[str, EnumInfo]) -> str:
    lines = [
        f"struct {c.name}_PropertyRegistrar {{",
        f"    {c.name}_PropertyRegistrar() {{",
        f"        using ThisClass = {c.name};",
        f"        UClass* Cls = {c.name}::StaticClass();",
        f"        (void)Cls;",
    ]
    for p in c.properties:
        lines.append("        " + emit_property_call(p, known_enums))
    lines.append("    }")
    lines.append("};")
    lines.append(f"static {c.name}_PropertyRegistrar s_{c.name}_PropertyReg;\n")
    return "\n".join(lines)


def emit_property_call(p: PropertyInfo, known_enums: dict[str, EnumInfo]) -> str:
    flags = " | ".join(p.flags)
    cat = f'"{p.category}"'
    disp = p.display_name or p.name  # PostEditProperty/editor keys off display name

    if p.prop_type == "EPropertyType::Float":
        return (
            f'PROPERTY_FLOAT({p.name}, "{disp}", {cat}, '
            f'{p.min or "0.0f"}, {p.max or "0.0f"}, {p.speed or "0.1f"}, {flags})'
        )

    if p.prop_type == "EPropertyType::Bool":
        return f'PROPERTY_BOOL({p.name}, "{disp}", {cat}, {flags})'

    if p.prop_type == "EPropertyType::Int":
        return f'PROPERTY_INT({p.name}, "{disp}", {cat}, {flags})'

    if p.prop_type == "EPropertyType::Vec3":
        return f'PROPERTY_VEC3({p.name}, "{disp}", {cat}, {flags})'

    if p.prop_type == "EPropertyType::String":
        return f'PROPERTY_STRING({p.name}, "{disp}", {cat}, {flags})'

    if p.prop_type == "EPropertyType::Array":
        if not p.array_inner_type:
            raise CodegenError(f"TArray property {p.name} has no inner type")
        inner_et, _, _ = classify_type(p.array_inner_type, known_enums)
        return (
            f'PROPERTY_ARRAY({p.name}, "{disp}", {cat}, {flags}, '
            f'{p.array_inner_type}, {inner_et}, (void)0)'
        )

    if p.prop_type == "EPropertyType::Enum":
        if not (p.enum_names and p.enum_count and p.enum_size):
            raise CodegenError(
                f"enum property {p.name}: v1 requires EnumNames=, EnumCount=, EnumSize= attributes"
            )
        return (
            f'PROPERTY_ENUM({p.name}, "{disp}", {cat}, '
            f'{p.enum_names}, {p.enum_count}, {p.enum_size}, {flags})'
        )

    if p.prop_type == "EPropertyType::Struct":
        if not p.struct_func:
            raise CodegenError(f"struct property {p.name}: v1 requires StructFunc= attribute")
        return f'PROPERTY_STRUCT({p.name}, "{disp}", {cat}, {p.struct_func}, {flags})'

    # Fallback for asset refs (StaticMeshRef etc.) and any explicit Type=
    return f'REGISTER_PROPERTY({p.name}, "{disp}", {p.prop_type}, {cat}, {flags})'


_LUA_PREFIX_STRIP = re.compile(r"^[UAF][A-Z]")

def lua_class_name(cpp_name: str) -> str:
    """Strip leading U/A/F prefix to match existing hand-binding convention
    (UActionComponent → ActionComponent, AActor → Actor, FVector → Vector)."""
    return cpp_name[1:] if _LUA_PREFIX_STRIP.match(cpp_name) else cpp_name


def emit_lua_registrar(c: ClassInfo) -> str:
    """Returns "" when the class has no UFUNCTION(Lua) members — bare UFUNCTION()
    is reserved for future consumers (CallInEditor, Exec) and does not bind to Lua."""
    lua_funcs = [fn for fn in c.functions if fn.is_lua]
    if not lua_funcs:
        return ""

    lua_name = lua_class_name(c.name)
    lines = [
        f"static void {c.name}_RegisterLua(sol::state& Lua) {{",
        f'    Lua.new_usertype<{c.name}>("{lua_name}",',
        f"        sol::base_classes, sol::bases<{c.parent}>(),",
    ]
    for i, fn in enumerate(lua_funcs):
        exposed = fn.lua_name or fn.name
        comma = "," if i < len(lua_funcs) - 1 else ""
        lines.append(f'        "{exposed}", &{c.name}::{fn.name}{comma}')
    lines.append("    );")
    lines.append("}")
    lines.append(f"static FLuaClassRegistrar s_{c.name}_LuaReg(&{c.name}_RegisterLua);\n")
    return "\n".join(lines)


# ──────────────────────────────────────────────
# Include Path Resolution
# ──────────────────────────────────────────────
def make_include_path(header: Path) -> str:
    for root in INCLUDE_ROOTS:
        try:
            rel = header.relative_to(root)
            return rel.as_posix()
        except ValueError:
            continue
    raise CodegenError(f"header {header} not under any include root")


# ──────────────────────────────────────────────
# Output
# ──────────────────────────────────────────────
def write_if_different(path: Path, content: str) -> bool:
    new_bytes = content.encode("utf-8")
    if path.exists() and path.read_bytes() == new_bytes:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(new_bytes)
    return True


def check_collisions(headers: list[Path]) -> None:
    seen: dict[str, Path] = {}
    for h in headers:
        if h.stem in seen:
            raise CodegenError(
                f"output collision: {h} and {seen[h.stem]} both produce "
                f"{h.stem}.generated.h — rename one"
            )
        seen[h.stem] = h


# ──────────────────────────────────────────────
# Entry Point
# ──────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--verbose", "-v", action="store_true")
    args = parser.parse_args()

    if args.clean:
        for d in (OUT_INC, OUT_SRC):
            if d.exists():
                for f in d.iterdir():
                    f.unlink()

    headers = discover_headers()
    check_collisions(headers)
    known_enums = build_enum_registry(headers)

    written = 0
    for h in headers:
        classes = parse_header(h, known_enums)
        if not classes:
            continue
        gh_text  = emit_generated_header(classes)
        cpp_text = emit_gen_cpp(make_include_path(h), classes, known_enums)

        if write_if_different(OUT_INC / f"{h.stem}.generated.h", gh_text):
            written += 1
            if args.verbose:
                print(f"  wrote {h.stem}.generated.h")
        if write_if_different(OUT_SRC / f"{h.stem}.gen.cpp", cpp_text):
            written += 1
            if args.verbose:
                print(f"  wrote {h.stem}.gen.cpp")

    print(f"GenerateCode: scanned {len(headers)} headers, wrote {written} files.")


if __name__ == "__main__":
    try:
        main()
    except CodegenError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)
