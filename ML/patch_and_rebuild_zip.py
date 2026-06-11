import os
import zipfile
import shutil

ZIP_PATH = r"C:\Users\REVANTH VISHNU REDDY\Desktop\IOT\TensorFlowLite_ESP32.zip"
TEMP_DIR = "temp_patch"

def patch_file(filepath):
    print(f"[+] Patching file: {filepath}")
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    # Replace the const count_ declaration
    target = "  const size_type count_;"
    replacement = "  size_type count_;"
    
    if target in content:
        content = content.replace(target, replacement)
        print("    Successfully replaced 'const size_type count_;'")
    else:
        print("    WARNING: 'const size_type count_;' not found, checking alternatives...")
        # Check for size_t fallback
        target2 = "  const size_t count_;"
        if target2 in content:
            content = content.replace(target2, "  size_t count_;")
            print("    Successfully replaced 'const size_t count_;'")
        else:
            print("    ERROR: Could not find target count_ declaration!")
            return False

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    return True

def patch_compatibility_file(filepath):
    print(f"[+] Patching compatibility file: {filepath}")
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    target = "#define TF_LITE_REMOVE_VIRTUAL_DELETE \\\n  void operator delete(void* p) {}"
    replacement = "#define TF_LITE_REMOVE_VIRTUAL_DELETE"
    
    if target.replace('\n', '\r\n') in content:
        content = content.replace(target.replace('\n', '\r\n'), replacement)
        print("    Successfully patched compatibility.h (windows line endings)")
    elif target in content:
        content = content.replace(target, replacement)
        print("    Successfully patched compatibility.h (unix line endings)")
    else:
        print("    ERROR: TF_LITE_REMOVE_VIRTUAL_DELETE not found in compatibility.h!")
        return False
        
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    return True

def zipdir(path, ziph):
    # ziph is zipfile.ZipFile object
    for root, dirs, files in os.walk(path):
        for file in files:
            file_path = os.path.join(root, file)
            # Write file with relative path matching the folder structure in ZIP
            arcname = os.path.relpath(file_path, path)
            # Prepend the root folder name to match original zip format
            arcname = os.path.join("Arduino_TensorFlowLite_ESP32-master", arcname)
            ziph.write(file_path, arcname)

def main():
    if not os.path.exists(ZIP_PATH):
        print(f"[-] ZIP file not found at {ZIP_PATH}!")
        return

    # 1. Clean and create temp directory
    if os.path.exists(TEMP_DIR):
        shutil.rmtree(TEMP_DIR)
    os.makedirs(TEMP_DIR, exist_ok=True)

    # 2. Extract ZIP
    print("[+] Extracting ZIP...")
    with zipfile.ZipFile(ZIP_PATH, 'r') as zip_ref:
        zip_ref.extractall(TEMP_DIR)

    # 3. Locate and patch stl_emulation.h
    stl_emulation_path = os.path.join(
        TEMP_DIR, 
        "Arduino_TensorFlowLite_ESP32-master", 
        "src", 
        "third_party", 
        "flatbuffers", 
        "stl_emulation.h"
    )

    if not os.path.exists(stl_emulation_path):
        # Fallback to find stl_emulation.h anywhere in temp
        for root, dirs, files in os.walk(TEMP_DIR):
            if "stl_emulation.h" in files:
                stl_emulation_path = os.path.join(root, "stl_emulation.h")
                break

    success = True
    if os.path.exists(stl_emulation_path):
        success = patch_file(stl_emulation_path)
    else:
        print("[-] ERROR: stl_emulation.h not found in extracted files!")
        success = False

    # Patch compatibility.h
    compatibility_path = os.path.join(
        TEMP_DIR, 
        "Arduino_TensorFlowLite_ESP32-master", 
        "src", 
        "tensorflow", 
        "lite", 
        "micro", 
        "compatibility.h"
    )

    if not os.path.exists(compatibility_path):
        for root, dirs, files in os.walk(TEMP_DIR):
            if "compatibility.h" in files:
                compatibility_path = os.path.join(root, "compatibility.h")
                break

    if os.path.exists(compatibility_path):
        success = success and patch_compatibility_file(compatibility_path)
    else:
        print("[-] ERROR: compatibility.h not found in extracted files!")
        success = False

    if success:
        extracted_root = os.path.join(TEMP_DIR, "Arduino_TensorFlowLite_ESP32-master")
        
        # Remove the screen driver directory to avoid LCD compilation errors
        screen_dir = os.path.join(extracted_root, "src", "screen")
        if os.path.exists(screen_dir):
            print("[+] Removing unused screen driver directory: ", screen_dir)
            shutil.rmtree(screen_dir)
            
        # Remove the bus driver directory to avoid legacy I2C conflict errors
        bus_dir = os.path.join(extracted_root, "src", "bus")
        if os.path.exists(bus_dir):
            print("[+] Removing unused bus driver directory: ", bus_dir)
            shutil.rmtree(bus_dir)
            
        # 4. Re-zip the library
        print("[+] Re-packing ZIP...")
        new_zip_path = ZIP_PATH + ".tmp"
        
        with zipfile.ZipFile(new_zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
            zipdir(extracted_root, zipf)
            
        # Overwrite the original zip file
        shutil.move(new_zip_path, ZIP_PATH)
        print("\n=======================================================")
        print("[+] TensorFlow Lite ZIP successfully patched and re-created!")
        print(f"    ZIP location: {ZIP_PATH}")
        print("=======================================================")
    else:
        print("[-] Library patch failed.")

    # 5. Clean up
    if os.path.exists(TEMP_DIR):
        shutil.rmtree(TEMP_DIR)

if __name__ == "__main__":
    main()
