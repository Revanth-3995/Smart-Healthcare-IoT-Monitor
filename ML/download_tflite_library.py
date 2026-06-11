import os
import urllib.request
import zipfile
import shutil

URL = "https://github.com/tanakamasayuki/Arduino_TensorFlowLite_ESP32/archive/refs/heads/master.zip"
ZIP_NAME = "tflite_library.zip"
TARGET_DIR = r"C:\Users\REVANTH VISHNU REDDY\Documents\Arduino\libraries\TensorFlowLite_ESP32"

def main():
    print(f"[+] Downloading TensorFlowLite_ESP32 from {URL}...")
    headers = {'User-Agent': 'Mozilla/5.0'}
    req = urllib.request.Request(URL, headers=headers)
    
    try:
        with urllib.request.urlopen(req) as response:
            with open(ZIP_NAME, 'wb') as f:
                f.write(response.read())
        print("[+] Download complete. Extracting...")
        
        temp_dir = "temp_library"
        os.makedirs(temp_dir, exist_ok=True)
        with zipfile.ZipFile(ZIP_NAME, 'r') as zip_ref:
            zip_ref.extractall(temp_dir)
            
        print("[+] Extracted files in temp_library:", os.listdir(temp_dir))
        
        extracted_folder = os.path.join(temp_dir, "Arduino_TensorFlowLite_ESP32-master")
        if not os.path.exists(extracted_folder):
            for item in os.listdir(temp_dir):
                if "TensorFlowLite" in item:
                    extracted_folder = os.path.join(temp_dir, item)
                    break
        
        print(f"[+] Extracted folder path: {extracted_folder} (exists: {os.path.exists(extracted_folder)})")
                    
        if os.path.exists(TARGET_DIR):
            shutil.rmtree(TARGET_DIR)
            
        shutil.copytree(extracted_folder, TARGET_DIR)
        
        # Clean up
        os.remove(ZIP_NAME)
        shutil.rmtree(temp_dir)
        
        print("\n=======================================================")
        print("[+] TensorFlowLite_ESP32 successfully installed!")
        print(f"    Installed at: {TARGET_DIR}")
        print("=======================================================")
    except Exception as e:
        print(f"\n[-] ERROR failed installing library: {e}")
        if os.path.exists(ZIP_NAME):
            os.remove(ZIP_NAME)
        if os.path.exists("temp_library"):
            shutil.rmtree("temp_library")
        raise e

if __name__ == "__main__":
    main()
