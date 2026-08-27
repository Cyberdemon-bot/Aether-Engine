#!/usr/bin/env python3
import os
import sys
import argparse
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent if SCRIPT_DIR.name == "scripts" else SCRIPT_DIR

BASE_DIR = PROJECT_ROOT / "Aether" / "src"
DEFAULT_OUTPUT_DIR = SCRIPT_DIR / "output"
DEFAULT_OUTPUT_FILE = "output.txt"

EXTENSIONS = {'.h', '.cpp', '.hpp', '.lua', '.shader'}
IGNORE_DIRS = {'vendor', 'build', '.xmake', '.git', 'bin', 'models', 'textures'}

def pack_files(target_folder: str, output_filename: str, output_dir: Path, mode: str):
    search_path = BASE_DIR / target_folder if target_folder else BASE_DIR

    if not search_path.exists():
        print(f"[X] Error: Directory does not exist: {search_path}")
        sys.exit(1)

    output_dir.mkdir(parents=True, exist_ok=True)
    output_path = output_dir / output_filename

    file_mode = 'a' if mode == 'append' else 'w'
    file_count = 0

    with open(output_path, file_mode, encoding="utf-8") as outfile:
        for root, dirs, files in os.walk(search_path):
            dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]
            
            for file in files:
                if any(file.endswith(ext) for ext in EXTENSIONS):
                    path = Path(root) / file
                    
                    if "pack_code.py" in path.name or output_filename in path.name:
                        continue
                        
                    outfile.write(f"\n\n{'='*20}\nFILE: {path}\n{'='*20}\n")
                    try:
                        with open(path, "r", encoding="utf-8", errors="ignore") as infile:
                            outfile.write(infile.read())
                        file_count += 1
                    except Exception as e:
                        outfile.write(f"[Error reading file: {e}]")

    print(f"[✓] Successfully packed {file_count} files from '{search_path}' into '{output_path}' (Mode: {mode.upper()}).")

def main():
    parser = argparse.ArgumentParser(
        description="Aether Source Code Automation Packer Tool",
        epilog="Example: python scripts/pack_code.py Renderer --mode clear"
    )
    
    parser.add_argument(
        "folder", 
        nargs="?", 
        default="", 
        help="Subfolder inside 'Aether/src/Aether' to pack. Leave empty to pack the entire 'Aether/src/Aether' directory."
    )
    
    parser.add_argument(
        "-m", "--mode", 
        choices=["clear", "append"], 
        default="clear", 
        help="Write mode: 'clear' (overwrite) or 'append' (append to existing file). Default is 'clear'."
    )
    
    parser.add_argument(
        "-o", "--output", 
        default=DEFAULT_OUTPUT_FILE, 
        help=f"Output filename. Default is '{DEFAULT_OUTPUT_FILE}'."
    )

    parser.add_argument(
        "-d", "--dir", 
        type=Path,
        default=DEFAULT_OUTPUT_DIR, 
        help=f"Target output directory. Default is '{DEFAULT_OUTPUT_DIR}'."
    )

    args = parser.parse_args()
    pack_files(args.folder, args.output, args.dir, args.mode)

if __name__ == "__main__":
    main()