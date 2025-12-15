#!/usr/bin/env python3
import argparse
import os
import json
import subprocess
import platform
import re
import sys
import struct
import time

class ShaderPackBytecode:
    def __init__(self, bytecode: bytes, data_size: int):
        self.bytecode = bytecode
        self.data_size = data_size

def main():
    parser = argparse.ArgumentParser(description="Process shader packs")
    parser.add_argument("--packs", nargs='+', required=True, help="Space-delimited list of shader packs")
    args = parser.parse_args()

    cwd = os.getcwd()
    build_dir = os.path.join(cwd, "build")

    os.makedirs(build_dir, exist_ok=True)

    print("-- Running shader compiler")

    shader_compiler = os.path.join(cwd, "shader_compiler.exe")
    packs_arg = " ".join(args.packs)
    command = f"{shader_compiler} /P {cwd} {packs_arg}"

    if platform.system() == "Linux":
        command = f"wine {command}"

    # Start the timer
    start_time = time.time()

    try:
        process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        stdout, _ = process.communicate()
        process.wait()
        
        # Stop the timer and calculate the compilation time in seconds with two decimal places
        end_time = time.time()
        compilation_time = "{:.2f}sec".format(end_time - start_time)
        
        if process.returncode != 0:
            print(f"Error running shader compiler: {stdout.decode('utf-8')}")
            sys.exit(1)
        else:
            # Initialize a counter for the number of packed shaders
            packed_shaders_count = 0
            
            # Parse and print the output
            output_lines = stdout.decode('utf-8').splitlines()
            for line in output_lines:
                match = re.search(r'(\w+): updated (\d+) shaders', line)
                if match:
                    shader_type = match.group(1)
                    updated_shaders = match.group(2)
                    packed_shaders_count += int(updated_shaders)  # Update the counter
                    print(f"-- Updated {updated_shaders} shaders in {shader_type}")
            
            shader_plural = "shader" if packed_shaders_count == 1 else "shaders"
            print(f"-- Compiled and packed {packed_shaders_count} {shader_plural} in {compilation_time} {packed_shaders_count == 0 and '(no new changes)' or ''}")

            # Move the generated files to the build directory
            for pack in args.packs:
                with open(os.path.join(cwd, f'shaders_{pack}.pack'), 'rb') as f:
                    with open(os.path.join(build_dir, f'shaders_{pack}.pack'), 'wb') as f2:
                        f2.write(f.read())

            print(f"-- Built shader pack files available in '{build_dir}'")
    except subprocess.CalledProcessError as e:
        print(f"Error running shader compiler: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()