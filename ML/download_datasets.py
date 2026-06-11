import os
import urllib.request
import zipfile

HAPT_URL = "https://archive.ics.uci.edu/ml/machine-learning-databases/00341/HAPT%20Data%20Set.zip"
SISFALL_URL = "https://github.com/BIng2325/SisFall/releases/download/dataset/SisFall.zip"

DATASETS_DIR = "datasets"
HAPT_ZIP = "hapt.zip"
SISFALL_ZIP = "sisfall.zip"

def download_and_extract(url, zip_name, extract_dir):
    print(f"\n[+] Downloading from {url}...")
    headers = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'}
    req = urllib.request.Request(url, headers=headers)
    
    try:
        with urllib.request.urlopen(req) as response:
            total_size = int(response.headers.get('content-length', 0))
            block_size = 1024 * 64
            read_so_far = 0
            
            with open(zip_name, 'wb') as f:
                while True:
                    block = response.read(block_size)
                    if not block:
                        break
                    f.write(block)
                    read_so_far += len(block)
                    
                    if total_size > 0:
                        percent = min(100, read_so_far * 100 / total_size)
                        mb_read = read_so_far / (1024 * 1024)
                        mb_total = total_size / (1024 * 1024)
                        print(f"\r    Progress: {percent:.1f}% ({mb_read:.1f} MB / {mb_total:.1f} MB)", end="", flush=True)
                    else:
                        mb_read = read_so_far / (1024 * 1024)
                        print(f"\r    Downloaded: {mb_read:.1f} MB", end="", flush=True)
                        
            print("\n[+] Download complete. Extracting file...")
            
            os.makedirs(extract_dir, exist_ok=True)
            with zipfile.ZipFile(zip_name, 'r') as zip_ref:
                uncompress_size = sum((file.file_size for file in zip_ref.infolist()))
                extracted_size = 0
                for file in zip_ref.infolist():
                    zip_ref.extract(file, extract_dir)
                    extracted_size += file.file_size
                    percent = (extracted_size / uncompress_size) * 100 if uncompress_size > 0 else 100
                    print(f"\r    Extracting: {percent:.1f}%", end="", flush=True)
                    
            print("\n[+] Extraction complete.")
    except Exception as e:
        print(f"\n[-] ERROR failed downloading/extracting: {e}")
        if os.path.exists(zip_name):
            os.remove(zip_name)
        raise e
        
    try:
        os.remove(zip_name)
        print(f"[+] Cleaned up temporary zip: {zip_name}")
    except OSError:
        pass

def main():
    os.makedirs(DATASETS_DIR, exist_ok=True)
    
    # 1. Download & Extract HAPT
    hapt_target_dir = os.path.join(DATASETS_DIR, "hapt")
    if os.path.exists(hapt_target_dir) and len(os.listdir(hapt_target_dir)) > 0:
        print("[*] HAPT dataset folder already exists. Skipping.")
    else:
        download_and_extract(HAPT_URL, HAPT_ZIP, hapt_target_dir)

    # 2. Download & Extract SisFall
    sisfall_target_dir = os.path.join(DATASETS_DIR, "sisfall")
    if os.path.exists(sisfall_target_dir) and len(os.listdir(sisfall_target_dir)) > 0:
        print("[*] SisFall dataset folder already exists. Skipping.")
    else:
        download_and_extract(SISFALL_URL, SISFALL_ZIP, sisfall_target_dir)

    print("\n=======================================================")
    print("[+] All dataset downloads and extractions completed successfully!")
    print(f"  - UCI HAPT: {os.path.abspath(hapt_target_dir)}")
    print(f"  - SisFall:  {os.path.abspath(sisfall_target_dir)}")
    print("=======================================================")

if __name__ == "__main__":
    main()
