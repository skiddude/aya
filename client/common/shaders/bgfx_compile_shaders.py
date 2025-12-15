#!/usr/bin/env python3
"""
Shader Packer for bgfx
Compiles all shaders from bgfx_shaders.json for multiple backends
"""

import json
import os
import subprocess
import sys
import tempfile
import struct
import hashlib
import re
from pathlib import Path
from typing import Dict, List, Optional

# Shader compiler configurations
SHADERC = "shaderc"  # bgfx shaderc compiler path

# Backend configurations
BACKENDS = {
    "spirv": {
        "vs": ["linux", "spirv16-13"],
        "fs": ["linux", "spirv16-13"],
    },
    "metal": {
        "vs": ["osx", "metal31-14"],
        "fs": ["osx", "metal31-14"],
    },
    "glsl": {
        "vs": ["linux", "430"],
        "fs": ["linux", "430"],
    },
    "glsles": {
        "vs": ["android", "310_es"],
        "fs": ["android", "310_es"],
    }
}


class ShaderCompiler:
    def __init__(self, source_dir: str, output_dir: str, include_dir: str):
        self.source_dir = Path(source_dir)
        self.output_dir = Path(output_dir)
        self.include_dir = Path(include_dir)
        self.varying_def = self.source_dir / "varying.def.sc"
        self.compiled_shaders = {}  # Track compiled shaders for packing
        
    def get_shader_type(self, name: str) -> str:
        """Determine shader type from name"""
        name_lower = name.lower()
        if "vs" in name_lower or "vertex" in name_lower:
            return "vertex"
        elif "fs" in name_lower or "fragment" in name_lower or "ps" in name_lower or "pixel" in name_lower:
            return "fragment"
        elif "cs" in name_lower or "compute" in name_lower:
            return "compute"
        else:
            # Default based on target
            return "vertex" if name_lower.endswith("vs") else "fragment"

    def resolve_includes(self, shader_source: str, source_path: Path, max_depth: int = 10) -> str:
        """
        Recursively resolve #include directives in shader source.
        Returns the shader source with all includes expanded inline.
        """
        if max_depth <= 0:
            print(f"  Warning: max include depth reached, possible circular include")
            return shader_source
        
        # Pattern to match: #include "filename" or #include <filename>
        include_pattern = re.compile(r'^\s*#\s*include\s+[<"]([^>"]+)[>"]', re.MULTILINE)
        
        def replace_include(match):
            include_file = match.group(1)
            # Try to find the include file relative to the source file or in include dir
            include_path = source_path.parent / include_file
            if not include_path.exists():
                include_path = self.include_dir / include_file
            if not include_path.exists():
                include_path = self.source_dir / include_file
            
            if not include_path.exists():
                # Can't find the file, leave the include directive as-is
                return match.group(0)
            
            try:
                with open(include_path, 'r') as f:
                    included_content = f.read()
                
                # Recursively resolve includes in the included file
                resolved_content = self.resolve_includes(included_content, include_path, max_depth - 1)
                
                # Return the included content with a comment indicating what was included
                return f"// BEGIN INCLUDE: {include_file}\n{resolved_content}\n// END INCLUDE: {include_file}\n"
            except Exception as e:
                print(f"  Warning: failed to include {include_file}: {e}")
                return match.group(0)
        
        return include_pattern.sub(replace_include, shader_source)

    def prune_unused_functions(self, shader_source: str, entrypoint: str) -> str:
        """
        Remove function definitions and prototypes not reachable from `entrypoint`.

        Heuristic-based static analysis:
        - Finds top-level function definitions (simple regex for return-type + name(...) { ... }).
        - Builds a call graph by scanning function bodies for function-name( tokens.
        - Performs BFS from `entrypoint` to find reachable functions.
        - Removes function definitions and prototypes not in the reachable set.

        This is intentionally conservative: if the entrypoint is not found or parsing fails,
        it returns the original source unchanged.
        """
        try:
            src = shader_source

            # Find function definitions using a heuristic regex; capture the function name
            # Pattern: optional leading qualifiers + return type, then function name, params, then '{'
            func_header_re = re.compile(r'^[ \t]*(?:[A-Za-z_][\w<>\s\*\:\&]+)\s+([A-Za-z_]\w*)\s*\([^;{]*\)\s*\{', re.MULTILINE)

            funcs = {}
            for m in func_header_re.finditer(src):
                name = m.group(1)
                # find the opening brace position for this match
                brace_pos = src.find('{', m.end() - 1)
                if brace_pos == -1:
                    continue

                # find matching closing brace
                depth = 0
                i = brace_pos
                end = None
                while i < len(src):
                    if src[i] == '{':
                        depth += 1
                    elif src[i] == '}':
                        depth -= 1
                        if depth == 0:
                            end = i
                            break
                    i += 1

                if end is None:
                    # Unbalanced braces, abort pruning
                    return shader_source

                # find start of the function definition (walk backwards to line start)
                start = src.rfind('\n', 0, m.start())
                start = start + 1 if start != -1 else 0

                funcs[name] = (start, end + 1, src[start:end + 1])

            if not funcs:
                return shader_source

            # Build call graph: for each function, find calls to other function names
            call_re = re.compile(r'\b([A-Za-z_]\w*)\s*\(')
            graph = {name: set() for name in funcs}
            for name, (_, _, body) in funcs.items():
                for call in call_re.findall(body):
                    if call != name and call in funcs:
                        graph[name].add(call)

            # BFS from entrypoint
            if entrypoint not in funcs:
                # entrypoint may be renamed with a suffix or have different case; try case-insensitive match
                lower_map = {n.lower(): n for n in funcs}
                if entrypoint.lower() in lower_map:
                    entrypoint = lower_map[entrypoint.lower()]
                else:
                    # No entrypoint found in parsed functions — don't prune
                    return shader_source

            reachable = set()
            stack = [entrypoint]
            while stack:
                cur = stack.pop()
                if cur in reachable:
                    continue
                reachable.add(cur)
                for nb in graph.get(cur, ()): 
                    if nb not in reachable:
                        stack.append(nb)

            # Determine functions to remove
            # Only remove shader entry points:
            # - Functions ending in VS, PS, FS, or CS (e.g., DefaultVS, AdornPS)
            # - Functions starting with vs or ps followed by uppercase or nothing (e.g., vsCustom, ps, psAdd)
            # Don't remove utility functions as they may be called by bgfx-generated code
            entry_point_pattern = re.compile(r'^([a-zA-Z][a-zA-Z0-9_]*(VS|PS|FS|CS)|vs($|[A-Z])|ps($|[A-Z])[a-zA-Z0-9_]*)$')
            to_remove = [
                name for name in funcs 
                if name not in reachable and entry_point_pattern.match(name)
            ]
            if not to_remove:
                return shader_source

            # Remove definitions in reverse order of start index
            removals = sorted(((funcs[n][0], funcs[n][1], n) for n in to_remove), key=lambda x: x[0], reverse=True)
            new_src = src
            for s, e, n in removals:
                # Remove the function definition span
                new_src = new_src[:s] + new_src[e:]

                # Also remove possible prototypes like: 'void name(...);' (single line)
                proto_re = re.compile(r'^\s*[\w\*\s\:\<\>]+\b' + re.escape(n) + r'\s*\([^;{]*\)\s*;\s*\n?', re.MULTILINE)
                new_src = proto_re.sub('', new_src)

            print(f"  Pruned {len(to_remove)} unused function(s): {', '.join(sorted(to_remove))}")
            return new_src

        except Exception as e:
            # Parsing failed — be safe and return original source
            print(f"  Warning: pruning failed ({e}), using original source")
            return shader_source
    
    def compile_shader(
        self,
        shader_name: str,
        source_file: str,
        entrypoint: str,
        backend: str,
        defines: Optional[List[str]] = None,
        exclude: Optional[str] = None,
        target: str = "",
    ) -> bool:
        """Compile a single shader variant"""
        
        # Check exclusions
        if exclude and backend in exclude.split():
            print(f"  Skipping {shader_name} for {backend} (excluded)")
            return True
        
        # Determine shader type
        shader_type = self.get_shader_type(target)
        type_flag = "vertex" if shader_type == "vertex" else "fragment"
        
        # Get backend config
        backend_config = BACKENDS.get(backend)
        if not backend_config:
            print(f"  Unknown backend: {backend}")
            return False
        
        # Determine platform and profile
        platform = backend_config[type_flag[0] + "s"][0]
        profile = backend_config[type_flag[0] + "s"][1]
        
        # Prepare output directory
        output_path = self.output_dir / backend / shader_type
        output_path.mkdir(parents=True, exist_ok=True)
        
        # Output file
        output_file = output_path / f"{shader_name}.bin"
        
        # Source file path
        source_path = self.source_dir / source_file
        if not source_path.exists():
            print(f"  ERROR: Source file not found: {source_path}")
            return False
        
        # Read the original shader source
        with open(source_path, 'r') as f:
            shader_source = f.read()
        
        # Resolve #includes to get the full source code for pruning
        resolved_source = self.resolve_includes(shader_source, source_path)
        
        # Prune unused functions to avoid including vertex shader code in fragment shaders
        pruned_source = self.prune_unused_functions(resolved_source, entrypoint)
        
        # For fragment shaders, replace $input with the varyings from $output
        if shader_type == "fragment":
            # Extract $output directive to get the varying list
            output_match = re.search(r'\$output\s+([^\n]+)', pruned_source)
            if output_match:
                # Get the varyings from $output
                varyings = output_match.group(1)
                # Replace $input with the varyings for fragment shaders
                # This ensures fragment shaders receive varyings, not vertex attributes
                pruned_source = re.sub(r'\$input\s+[^\n]+', f'$input {varyings}', pruned_source, count=1)


        # Create a temporary file with the main() wrapper
        # Instead of wrapping, we'll use preprocessor to rename the entry point to main
        temp_source = None
        try:
            # Create temporary file in the same directory as source for proper include paths
            temp_fd, temp_path = tempfile.mkstemp(
                suffix='.sc',
                prefix=f'tmp_{shader_name}_',
                dir=str(self.source_dir),
                text=True
            )
            temp_source = Path(temp_path)
            
            # Write shader source with preprocessor macro to rename entry point to main
            with os.fdopen(temp_fd, 'w') as f:
                # Replace the entry point function name with 'main' using preprocessor
                # This must be done BEFORE any includes so the function is named correctly
                f.write(f"// Auto-generated: Renaming {entrypoint} to main for bgfx\n")
                f.write(f"#define {entrypoint} main\n\n")
                
                # Now write the pruned shader content
                f.write(pruned_source)
            
            # Build shaderc command using the temporary file
            cmd = [
                SHADERC,
                "-f", str(temp_source),
                "-o", str(output_file),
                "--platform", platform,
                "--type", type_flag,
                "-i", str(self.include_dir),
                "--varyingdef", str(self.varying_def),
                "--profile", profile,
            ]
            
            # Add defines
            if defines:
                for define in defines.split():
                    cmd.extend(["--define", define])
            
            # Add verbose flag for debugging (optional)
            # cmd.append("--verbose")
            
            # Run shader compiler
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=False,
            )
            
            if result.returncode != 0:
                print(f"  ERROR compiling {shader_name} ({backend}):")
                print(f"    Command: {' '.join(cmd)}")
                print(f"    STDERR: {result.stderr}")
                if result.stdout:
                    print(f"    STDOUT: {result.stdout}")
                return False
            
            # Track compiled shader for packing
            if backend not in self.compiled_shaders:
                self.compiled_shaders[backend] = []
            self.compiled_shaders[backend].append({
                'name': shader_name,
                'path': output_file,
            })
            
            print(f"  ✓ {shader_name} ({backend}/{shader_type})")
            return True
            
        except FileNotFoundError:
            print(f"  ERROR: shaderc not found. Please ensure bgfx shaderc is in PATH or set SHADERC variable.")
            return False
        except Exception as e:
            print(f"  ERROR: {e}")
            return False
        finally:
            # Clean up temporary file
            if temp_source and temp_source.exists():
                try:
                    temp_source.unlink()
                except:
                    pass
    
    def create_pack(self, backend: str, pack_name: str) -> bool:
        """Create a shader pack file in the RBXS format"""
        
        if backend not in self.compiled_shaders:
            print(f"  No compiled shaders for backend: {backend}")
            return False
        
        shaders = self.compiled_shaders[backend]
        if not shaders:
            print(f"  No shaders to pack for backend: {backend}")
            return False
        
        print(f"\nCreating pack: {pack_name}")
        
        # Pack format:
        # - Header: "RBXS" (4 bytes)
        # - Entry count: unsigned int (4 bytes)
        # - Entry table: array of PackEntryFile (92 bytes each)
        # - Data: concatenated shader binaries
        
        # PackEntryFile structure (92 bytes total):
        # - name: char[64]
        # - md5: char[16]
        # - offset: unsigned int (4 bytes)
        # - size: unsigned int (4 bytes)
        # - reserved: char[8]
        
        entries = []
        data_offset = 8 + (92 * len(shaders))  # Header + count + entry table
        shader_data = bytearray()
        
        for shader in sorted(shaders, key=lambda x: x['name']):
            shader_path = shader['path']
            shader_name = shader['name']
            
            # Read shader binary
            if not shader_path.exists():
                print(f"  ERROR: Compiled shader not found: {shader_path}")
                return False
            
            with open(shader_path, 'rb') as f:
                binary_data = f.read()
            
            # Calculate MD5
            md5 = hashlib.md5(binary_data).digest()
            
            # Create entry
            entry = {
                'name': shader_name.encode('utf-8')[:64].ljust(64, b'\0'),
                'md5': md5,
                'offset': data_offset + len(shader_data),
                'size': len(binary_data),
                'reserved': b'\0' * 8,
            }
            
            entries.append(entry)
            shader_data.extend(binary_data)
            
            print(f"  + {shader_name} ({len(binary_data)} bytes)")
        
        # Write pack file
        pack_path = self.output_dir / pack_name
        
        try:
            with open(pack_path, 'wb') as f:
                # Write header
                f.write(b'RBXS')
                
                # Write entry count
                f.write(struct.pack('<I', len(entries)))
                
                # Write entry table
                for entry in entries:
                    f.write(entry['name'])
                    f.write(entry['md5'])
                    f.write(struct.pack('<I', entry['offset']))
                    f.write(struct.pack('<I', entry['size']))
                    f.write(entry['reserved'])
                
                # Write shader data
                f.write(shader_data)
            
            print(f"\n✓ Created pack: {pack_path} ({len(entries)} shaders, {len(shader_data)} bytes)")
            return True
            
        except Exception as e:
            print(f"  ERROR creating pack: {e}")
            return False
    
    def pack_all_shaders(self, backends: Optional[List[str]] = None):
        """Compile all shaders from bgfx_shaders.json"""

        # Read bgfx_shaders.json
        shaders_json = self.source_dir / "../bgfx_shaders.json"
        if not shaders_json.exists():
            print(f"ERROR: bgfx_shaders.json not found at {shaders_json}")
            return False
        
        with open(shaders_json, "r") as f:
            shaders = json.load(f)
        
        # Use all backends if none specified
        if backends is None:
            backends = list(BACKENDS.keys())
        
        print(f"Compiling {len(shaders)} shader variants for backends: {', '.join(backends)}")
        print(f"Source: {self.source_dir}")
        print(f"Output: {self.output_dir}")
        print()
        
        total = 0
        success = 0
        failed = []
        
        for shader in shaders:
            name = shader["name"]
            source = shader["source"]
            entrypoint = shader["entrypoint"]
            defines = shader.get("defines", "")
            exclude = shader.get("exclude", "")
            target = shader.get("target", "")

            print(f"Compiling {name} from {source}::{entrypoint}")
            
            for backend in backends:
                total += 1
                if self.compile_shader(name, source, entrypoint, backend, defines, exclude, target):
                    success += 1
                else:
                    failed.append(f"{name} ({backend})")
        
        print()
        print(f"Compilation complete: {success}/{total} successful")
        
        if failed:
            print(f"\nFailed shaders:")
            for f in failed:
                print(f"  - {f}")
            return False
        
        # Create pack files for each backend
        print("\n" + "="*60)
        print("Creating shader packs...")
        print("="*60)
        
        pack_success = True
        for backend in backends:
            # Map backend names to pack names
            pack_name_map = {
                "glsl": "shaders_glsl.pack",
                "glsles": "shaders_glsles.pack",
                "spirv": "shaders_spirv.pack",
                "osx": "shaders_osx.pack",
                "ios": "shaders_ios.pack"
            }
            
            pack_name = pack_name_map.get(backend, f"shaders_{backend}.pack")
            
            if not self.create_pack(backend, pack_name):
                pack_success = False
        
        return pack_success


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Compile bgfx shaders from bgfx_shaders.json")
    parser.add_argument(
        "--source",
        default="bgfx_source",
        help="Source directory containing .sc files (default: bgfx_source)",
    )
    parser.add_argument(
        "--output",
        default="compiled",
        help="Output directory for compiled shaders (default: compiled)",
    )
    parser.add_argument(
        "--include",
        default="bgfx_source",
        help="Include directory for shader headers (default: bgfx_source)",
    )
    parser.add_argument(
        "--packs",
        nargs="+",
        choices=list(BACKENDS.keys()),
        help="Backends to compile for (default: all)",
    )
    parser.add_argument(
        "--shaderc",
        default="shaderc",
        help="Path to bgfx shaderc compiler (default: shaderc in PATH)",
    )
    
    args = parser.parse_args()
    
    # Set global shaderc path
    global SHADERC
    SHADERC = args.shaderc
    
    # Get script directory
    script_dir = Path(__file__).parent
    
    # Resolve paths
    source_dir = script_dir / args.source
    output_dir = script_dir / args.output
    include_dir = script_dir / args.include
    
    # Create compiler
    compiler = ShaderCompiler(str(source_dir), str(output_dir), str(include_dir))
    
    # Compile shaders
    success = compiler.pack_all_shaders(args.packs)
    
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
