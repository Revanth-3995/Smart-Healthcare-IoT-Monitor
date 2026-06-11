import os
import urllib.request

URL = "https://github.com/tanakamasayuki/Arduino_TensorFlowLite_ESP32/archive/refs/heads/master.zip"
TARGET_PATH = r"C:\Users\REVANTH VISHNU REDDY\Desktop\IOT\TensorFlowLite_ESP32.zip"

def main():
    print(f"[+] Downloading TensorFlow Lite ZIP from {URL}...")
    headers = {'User-Agent': 'Mozilla/5.0'}
    req = urllib.request.Request(URL, headers=headers)
    
    try:
        with urllib.request.urlopen(req) as response:
            with open(TARGET_PATH, 'wb') as f:
                f.write(response.read())
        print("\n=======================================================")
        print("[+] ZIP library successfully downloaded!")
        print(f"    Saved at: {TARGET_PATH}")
        print("=======================================================")
    except Exception as e:
        print(f"\n[-] ERROR downloading library: {e}")
        raise e

if __name__ == "__main__":
    main()
