import os
import glob
import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.utils import resample

HAPT_DIR = "datasets/hapt"
SISFALL_DIR = "datasets/sisfall"

def check_directories():
    hapt_ok = os.path.exists(HAPT_DIR)
    sisfall_ok = os.path.exists(SISFALL_DIR)
    if not hapt_ok or not sisfall_ok:
        print("=================================================================")
        print("ERROR: Raw datasets not found. Please set up the folders:")
        if not hapt_ok:
            print(f"  - Download UCI HAPT and extract to: {os.path.abspath(HAPT_DIR)}")
        if not sisfall_ok:
            print(f"  - Download SisFall and extract to: {os.path.abspath(SISFALL_DIR)}")
        print("=================================================================")
        return False
    return True

def parse_hapt():
    print("[+] Processing UCI HAPT dataset...")
    # Map label paths
    labels_path = os.path.join(HAPT_DIR, "RawData", "labels.txt")
    if not os.path.exists(labels_path):
        labels_path = os.path.join(HAPT_DIR, "labels.txt")
    raw_dir = os.path.join(HAPT_DIR, "RawData")
    if not os.path.exists(raw_dir):
        raw_dir = HAPT_DIR

    # Load labels: [ExpID] [UserID] [ActivityID] [Start] [End]
    labels_list = []
    with open(labels_path, 'r') as f:
        for line in f:
            parts = list(map(int, line.strip().split()))
            if len(parts) >= 5:
                labels_list.append(parts)

    data_rows = []
    
    # Activity mapping
    # 0 = Normal, 1 = Resting, 2 = Bed Exit, 3 = Inactive
    hapt_mapping = {
        1: 0, 2: 0, 3: 0,   # Walking, Walking Up, Walking Down -> Normal
        4: 1, 5: 1,         # Sitting, Standing -> Resting
        10: 2, 12: 2,       # Lie to Sit, Lie to Stand -> Bed Exit
        6: 3                # Laying -> Inactive
    }

    # Track parsed files to avoid reloading the same file multiple times unnecessarily
    loaded_sessions = {}

    for exp_id, user_id, act_id, start, end in labels_list:
        if act_id not in hapt_mapping:
            continue
        
        target_class = hapt_mapping[act_id]
        session_key = (exp_id, user_id)
        
        if session_key not in loaded_sessions:
            acc_file = os.path.join(raw_dir, f"acc_exp{exp_id:02d}_user{user_id:02d}.txt")
            gyro_file = os.path.join(raw_dir, f"gyro_exp{exp_id:02d}_user{user_id:02d}.txt")
            
            if not os.path.exists(acc_file) or not os.path.exists(gyro_file):
                continue
                
            acc_data = np.loadtxt(acc_file)
            gyro_data = np.loadtxt(gyro_file)
            
            # Convert gyroscope from rad/s to deg/s
            gyro_data = gyro_data * (180.0 / np.pi)
            loaded_sessions[session_key] = (acc_data, gyro_data)
        
        acc_data, gyro_data = loaded_sessions[session_key]
        
        # Extract slices (HAPT labels start at index 1)
        slice_start = start - 1
        slice_end = min(end, len(acc_data), len(gyro_data))
        
        for i in range(slice_start, slice_end):
            data_rows.append([
                acc_data[i, 0], acc_data[i, 1], acc_data[i, 2], # ax, ay, az (in g)
                gyro_data[i, 0], gyro_data[i, 1], gyro_data[i, 2], # gx, gy, gz (in deg/s)
                target_class
            ])
            
    print(f"    Loaded {len(data_rows)} samples from HAPT.")
    return pd.DataFrame(data_rows, columns=["accel_x", "accel_y", "accel_z", "gyro_x", "gyro_y", "gyro_z", "label"])

def parse_sisfall():
    print("[+] Processing SisFall dataset (Falls)...")
    # SisFall contains files starting with 'F' for Falls and 'D' for ADLs
    fall_files = glob.glob(os.path.join(SISFALL_DIR, "**", "F*.txt"), recursive=True)

    data_rows = []

    # ADXL345 Scale: 13-bit, +-16g -> 32g range / 8192 LSB = 0.00390625 g/LSB
    ACCEL_SCALE = 0.00390625
    # ITG3200 Scale: 16-bit, +-2000 deg/s -> 4000 range / 65536 LSB = 0.061035156 deg/s/LSB
    GYRO_SCALE = 0.061035156

    for file_path in fall_files:
        with open(file_path, 'r') as f:
            for line in f:
                line = line.strip().replace(';', '')
                if not line:
                    continue
                parts = line.split(',')
                if len(parts) >= 6:
                    try:
                        # Columns 0, 1, 2: ADXL345 accelerometer counts
                        ax = int(parts[0]) * ACCEL_SCALE
                        ay = int(parts[1]) * ACCEL_SCALE
                        az = int(parts[2]) * ACCEL_SCALE
                        # Columns 3, 4, 5: ITG3200 gyroscope counts
                        gx = int(parts[3]) * GYRO_SCALE
                        gy = int(parts[4]) * GYRO_SCALE
                        gz = int(parts[5]) * GYRO_SCALE
                        
                        data_rows.append([ax, ay, az, gx, gy, gz, 4]) # 4 = Fall
                    except ValueError:
                        continue
                        
    print(f"    Loaded {len(data_rows)} samples from SisFall (Falls).")
    return pd.DataFrame(data_rows, columns=["accel_x", "accel_y", "accel_z", "gyro_x", "gyro_y", "gyro_z", "label"])

def augment_sensors(df):
    print("[+] Augmenting synthetic distance, PIR, and IR columns...")
    n = len(df)
    labels = df["label"].values
    
    distance = np.zeros(n)
    pir = np.zeros(n)
    ir = np.zeros(n)

    # Seed for reproducibility
    np.random.seed(42)

    for i in range(n):
        label = labels[i]
        if label == 0:  # Normal
            distance[i] = np.clip(np.random.normal(150.0, 50.0), 30.0, 300.0)
            pir[i] = 1.0 if np.random.rand() < 0.90 else 0.0
            ir[i] = 1.0 if np.random.rand() < 0.10 else 0.0
            
        elif label == 1:  # Resting
            distance[i] = np.clip(np.random.normal(120.0, 10.0), 30.0, 300.0)
            pir[i] = 1.0 if np.random.rand() < 0.15 else 0.0
            ir[i] = 1.0 if np.random.rand() < 0.30 else 0.0
            
        elif label == 2:  # Bed Exit
            # Transition simulation: linear spread between 30 and 100 with noise
            distance[i] = np.clip(np.random.uniform(30.0, 100.0) + np.random.normal(0.0, 5.0), 30.0, 300.0)
            pir[i] = 1.0 if np.random.rand() < 0.80 else 0.0
            ir[i] = 1.0 if np.random.rand() < 0.50 else 0.0
            
        elif label == 3:  # Inactive
            distance[i] = np.clip(np.random.normal(30.0, 0.5), 30.0, 300.0)
            pir[i] = 0.0 # No motion at all
            ir[i] = 0.0
            
        elif label == 4:  # Fall
            distance[i] = np.clip(np.random.normal(15.0, 2.0), 10.0, 300.0)
            pir[i] = 1.0 if np.random.rand() < 0.05 else 0.0 # tiny twitch
            ir[i] = 1.0 if np.random.rand() < 0.20 else 0.0

    df["distance_cm"] = distance
    df["pir_motion"] = pir
    df["ir_obstacle"] = ir
    return df

def balance_classes(df):
    print("[+] Balancing all 5 classes using resampling...")
    class_dfs = [df[df["label"] == i] for i in range(5)]
    
    # Cap target size to a reasonable amount (e.g., 50,000) to keep training fast and clean
    target_size = min(max(len(c) for c in class_dfs), 50000)
    print(f"    Target size per class (capped): {target_size} samples")
    
    balanced_dfs = []
    for i, c_df in enumerate(class_dfs):
        if len(c_df) == 0:
            print(f"    WARNING: Class {i} is empty! Cannot balance.")
            continue
        # Upsample if smaller, downsample if larger
        resampled_df = resample(c_df, replace=True, n_samples=target_size, random_state=42)
        balanced_dfs.append(resampled_df)
        
    return pd.concat(balanced_dfs).sample(frac=1.0, random_state=42).reset_index(drop=True)

def main():
    if not check_directories():
        return
        
    df_hapt = parse_hapt()
    df_sisfall = parse_sisfall()
    
    # Combine datasets
    df_combined = pd.concat([df_hapt, df_sisfall], ignore_index=True)
    
    # Synthesize additional features
    df_augmented = augment_sensors(df_combined)
    
    # Balance classes
    df_balanced = balance_classes(df_augmented)
    
    # Split into train/test
    X = df_balanced.drop(columns=["label"])
    y = df_balanced["label"]
    
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42, stratify=y)
    
    # Apply Standard Scaler
    scaler = StandardScaler()
    X_train_scaled = scaler.fit_transform(X_train)
    X_test_scaled = scaler.transform(X_test)
    
    # Save scalers as numpy files
    np.save("scaler_mean.npy", scaler.mean_)
    np.save("scaler_std.npy", scaler.scale_)
    
    # Reassemble and save CSV
    train_df = pd.DataFrame(X_train_scaled, columns=X.columns)
    train_df["label"] = y_train.values
    
    test_df = pd.DataFrame(X_test_scaled, columns=X.columns)
    test_df["label"] = y_test.values
    
    train_df.to_csv("train.csv", index=False)
    test_df.to_csv("test.csv", index=False)
    
    print("\n[+] Preprocessing Complete!")
    print("---------------------------------")
    print(f"Scaler Mean:\n{scaler.mean_}")
    print(f"Scaler Std:\n{scaler.scale_}")
    print("---------------------------------")
    print("Saved files: train.csv, test.csv, scaler_mean.npy, scaler_std.npy")

if __name__ == "__main__":
    main()
