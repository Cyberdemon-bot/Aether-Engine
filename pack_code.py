import os

# Các đuôi file muốn lấy
EXTENSIONS = {'.h', '.cpp', '.hpp', '.lua', '.shader'}
# Các thư mục muốn bỏ qua
IGNORE_DIRS = {'vendor', 'build', '.xmake', '.git', 'bin', 'models', 'textures'}

def pack_files():
    with open("full_source_code.txt", "w", encoding="utf-8") as outfile:
        # Duyệt qua thư mục src và file xmake.lua
        for root, dirs, files in os.walk("."):
            # Lọc bỏ thư mục không cần thiết
            dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]
            
            for file in files:
                if any(file.endswith(ext) for ext in EXTENSIONS):
                    path = os.path.join(root, file)
                    # Bỏ qua script này và file output
                    if "pack_code.py" in path or "full_source_code.txt" in path:
                        continue
                        
                    outfile.write(f"\n{'='*20}\nFILE: {path}\n{'='*20}\n")
                    try:
                        with open(path, "r", encoding="utf-8") as infile:
                            outfile.write(infile.read())
                    except Exception as e:
                        outfile.write(f"Error reading file: {e}")

if __name__ == "__main__":
    pack_files()
    print("Done!")