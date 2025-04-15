import absl.logging

absl.logging.set_verbosity(absl.logging.ERROR)

import os
import json
import numpy as np
from sklearn.model_selection import train_test_split, KFold
from sklearn.preprocessing import LabelEncoder
import tensorflow as tf

from model import create_model


def load_and_preprocess_data(data_dir):
    samples, labels = [], []

    # Collect labels and raw data
    for filename in os.listdir(data_dir):
        if filename.endswith(".json"):
            class_name = filename.split("_")[1]
            labels.append(class_name)
            with open(os.path.join(data_dir, filename), "r") as f:
                data = json.load(f)

            # Extract sensor data (5 sensors)
            sensor_ids = sorted([k for k in data.keys() if k != "init_time"])
            timesteps = min(len(data[sid]) for sid in sensor_ids)

            # Create 3D array: (timesteps, 5 sensors, 3 features)
            sample_data = np.zeros((timesteps, 5, 3))
            for i, sid in enumerate(sensor_ids):
                truncated = data[sid][:timesteps]
                sample_data[:, i, :] = [entry[2:5] for entry in truncated]

            samples.append(sample_data)

    # save a class_names.txt file
    class_names = sorted(set(labels))
    with open("class_names.txt", "w") as f:
        for name in class_names:
            f.write(f"{name}\n")

    # Return NumPy arrays instead of ragged tensors
    samples = np.array(samples, dtype=object)
    labels = LabelEncoder().fit_transform(labels)

    return samples, tf.keras.utils.to_categorical(labels)


def train(X_train, y_train, X_val, y_val, X_test, y_test):
    model = create_model(y.shape[1])
    model.compile(
        optimizer="adam", loss="categorical_crossentropy", metrics=["accuracy"]
    )

    X_train_ragged = tf.ragged.constant(X_train, ragged_rank=1)
    X_val_ragged = tf.ragged.constant(X_val, ragged_rank=1)
    X_test_ragged = tf.ragged.constant(X_test, ragged_rank=1)

    cbs = [
        tf.keras.callbacks.ModelCheckpoint(
            "model_weights.h5",
            monitor="val_loss",
            save_best_only=True,
            save_weights_only=True,
            mode="min",
            verbose=False,
        ),
        tf.keras.callbacks.ReduceLROnPlateau(
            monitor="val_loss",
            factor=0.5,
            patience=20,
            min_lr=1e-6,
            verbose=False,
        ),
        tf.keras.callbacks.EarlyStopping(
            monitor="val_loss",
            patience=50,
            restore_best_weights=True,
            verbose=False,
        ),
    ]

    model.fit(
        X_train_ragged,
        y_train,
        epochs=1000,
        validation_data=(X_val_ragged, y_val),
        callbacks=cbs,
    )

    model.load_weights("model_weights.h5")

    test_loss, test_acc = model.evaluate(X_test_ragged, y_test, verbose=0)
    print(f"Test accuracy: {test_acc:.4f}")

    return model, test_loss, test_acc


if __name__ == "__main__":
    data_dir = "./data"
    X, y = load_and_preprocess_data(data_dir)

    X_trainval, X_test, y_trainval, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y
    )
    print(f"train val size: {len(X_trainval)}, test size: {len(X_test)}")

    # X_train, X_val, y_train, y_val = train_test_split(
    #     X_trainval, y_trainval, test_size=0.2, random_state=42
    # )
    # print(f"train val size: {len(X_train)}, {len(X_val)}")
    # model = train(X_train, y_train, X_val, y_val)
    # model.save("model.pb")

    # k fold
    kf = KFold(n_splits=5, shuffle=True, random_state=42)
    results = []
    for fold, (train_index, val_index) in enumerate(kf.split(X_trainval)):
        print(f"Fold {fold + 1}")
        X_train, X_val = X_trainval[train_index], X_trainval[val_index]
        y_train, y_val = y_trainval[train_index], y_trainval[val_index]

        model, test_loss, test_acc = train(
            X_train, y_train, X_val, y_val, X_test, y_test
        )
        results.append((model, test_loss, test_acc))

    for i, (model, test_loss, test_acc) in enumerate(results):
        print(f"Fold {i + 1} Test Loss: {test_loss:.4f}, Test Accuracy: {test_acc:.4f}")

    # Save the best model
    best_model = min(results, key=lambda x: x[1])
    best_model[0].save("model.pb")
    print(f"Model with loss {best_model[1]:.4f} saved as model.pb")

    # saved_model_cli show --dir ./model.pb --tag_set serve --signature_def serving_default
