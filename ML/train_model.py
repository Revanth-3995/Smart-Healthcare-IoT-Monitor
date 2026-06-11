import numpy as np
import pandas as pd
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
from sklearn.metrics import classification_report, f1_score

def main():
    # Load dataset
    print("[+] Loading train.csv and test.csv...")
    try:
        train_df = pd.read_csv("train.csv")
        test_df = pd.read_csv("test.csv")
    except FileNotFoundError:
        print("ERROR: Preprocessed CSV files not found. Please run preprocess.py first.")
        return

    X_train = train_df.drop(columns=["label"]).values
    y_train = train_df["label"].values
    X_test = test_df.drop(columns=["label"]).values
    y_test = test_df["label"].values

    print(f"    Train shape: {X_train.shape}")
    print(f"    Test shape: {X_test.shape}")

    # Build model (Dense 16 -> Dense 8 -> Dense 5 softmax)
    model = Sequential([
        Dense(16, activation='relu', input_shape=(9,)),
        Dense(8, activation='relu'),
        Dense(5, activation='softmax')
    ])

    model.compile(
        optimizer='adam',
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )

    print("\n[+] Training lightweight Keras model...")
    # Train for 20 epochs with a batch size of 64
    model.fit(
        X_train, y_train,
        epochs=20,
        batch_size=64,
        validation_split=0.1,
        verbose=1
    )

    # Evaluate
    print("\n[+] Evaluating model on test set...")
    y_pred_probs = model.predict(X_test)
    y_pred = np.argmax(y_pred_probs, axis=1)

    print("\nClassification Report:")
    target_names = ["Normal (0)", "Resting (1)", "Bed Exit (2)", "Inactive (3)", "Fall (4)"]
    print(classification_report(y_test, y_pred, target_names=target_names))

    # Check fall class (class 4) F1-score
    fall_f1 = f1_score(y_test, y_pred, average=None)[4]
    print(f"Fall Class F1-Score: {fall_f1:.4f}")
    if fall_f1 < 0.85:
        print("WARNING: Fall class F1-Score is below target 0.85. Consider tuning or up-training.")
    else:
        print("[+] Success: Fall class F1-Score is above target threshold!")

    # Convert to TFLite
    print("\n[+] Converting Keras model to TFLite...")
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    tflite_model = converter.convert()

    # Save binary
    tflite_path = "model.tflite"
    with open(tflite_path, "wb") as f:
        f.write(tflite_model)
    print(f"    Saved TFLite model to {tflite_path} ({len(tflite_model)} bytes)")

    # Export to model.h C array
    print("\n[+] Generating C header file (model.h)...")
    header_path = "model.h"
    with open(header_path, "w") as f:
        f.write("#ifndef MODEL_H\n")
        f.write("#define MODEL_H\n\n")
        f.write("// Lightweight classification model: Normal, Resting, Bed Exit, Inactive, Fall\n")
        f.write(f"// Size: {len(tflite_model)} bytes\n\n")
        f.write("const unsigned char g_model[] __attribute__((aligned(8))) = {\n")
        
        # Format bytes as hex array
        hex_bytes = [f"0x{b:02x}" for b in tflite_model]
        for i in range(0, len(hex_bytes), 12):
            chunk = hex_bytes[i:i+12]
            f.write("    " + ", ".join(chunk) + ",\n")
            
        f.write("};\n\n")
        f.write(f"const unsigned int g_model_len = {len(tflite_model)};\n\n")
        f.write("#endif // MODEL_H\n")
        
    print(f"    Saved memory-aligned C array to {header_path}")

if __name__ == "__main__":
    main()
