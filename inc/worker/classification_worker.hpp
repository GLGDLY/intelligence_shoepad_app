#ifndef _CLASSIFICATION_WORKER_HPP
#define _CLASSIFICATION_WORKER_HPP

#include <QHash>
#include <QVector>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <tensorflow/c/c_api.h>

/*
	[
		#  Input shape: (None, 5, 3) = (variable_timesteps, sensors, features)
		tf.keras.layers.TimeDistributed(
			tf.keras.layers.Flatten(), input_shape=(None, 5, 3)
		),
		tf.keras.layers.LSTM(64, return_sequences=False),
		tf.keras.layers.Dense(32, activation="relu"),
		tf.keras.layers.Dense(num_classes, activation="softmax"),
	]
*/

typedef struct {
	float X, Y, Z;
} ClassificationDataPoint;

class ClassificationWorker : public QObject {
	Q_OBJECT
public:
	ClassificationWorker(QObject* parent = nullptr);
	~ClassificationWorker();
	void init(std::string model_path, std::string label_path);

	void addData(QString key, float X, float Y, float Z);

public slots:
	void classify(int index);
	void classifyCurrentSlot();

Q_SIGNALS:
	void sig_classify(int index);
	void sig_classificationResult(const QString& result);

private:
	TF_Graph* graph;
	TF_Session* session;
	TF_Status* status;
	TF_Buffer* run_options;
	TF_Buffer* run_metadata;
	TF_Output input_op;
	TF_Output output_op;
	QVector<QString> labels;

	QHash<QString, QVector<ClassificationDataPoint>> data_queue[2];
	int data_queue_index = 0;
	bool data_queue_available[2] = {true, true};
	const int max_data_size = 50;
};

#endif // _CLASSIFICATION_WORKER_HPP
