import tensorflow as tf


def create_model(num_classes: int) -> tf.keras.Model:
    model = tf.keras.models.Sequential(
        [
            #  Input shape: (None, 5, 3) = (variable_timesteps, sensors, features)
            tf.keras.layers.TimeDistributed(
                tf.keras.layers.Flatten(), input_shape=(None, 5, 3), name="tb"
            ),
            tf.keras.layers.LSTM(32, return_sequences=False),
            tf.keras.layers.Dense(16, activation="relu"),
            tf.keras.layers.Dense(num_classes, activation="softmax"),
        ]
    )
    return model


if __name__ == "__main__":
    model = create_model(num_classes=2)
    model.summary()
    tf.keras.utils.plot_model(
        model, to_file="model.png", show_shapes=True, show_layer_names=True
    )
