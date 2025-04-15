#include "worker/classification_worker.hpp"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <algorithm>
#include <chrono>
#include <fstream>

/*
The given SavedModel SignatureDef contains the following input(s):
  inputs['time_distributed_input'] tensor_info:
	  dtype: DT_FLOAT
	  shape: (-1, -1, 5, 3)
	  name: serving_default_time_distributed_input:0
The given SavedModel SignatureDef contains the following output(s):
  outputs['dense_1'] tensor_info:
	  dtype: DT_FLOAT
	  shape: (-1, 2)
	  name: StatefulPartitionedCall:0
Method name is: tensorflow/serving/predict
*/


/* Helper function */
template <typename T, typename A>
int argmax(std::vector<T, A> const& vec) {
	return static_cast<int>(std::distance(vec.begin(), max_element(vec.begin(), vec.end())));
}

/* ClassificationWorker */
ClassificationWorker::ClassificationWorker(QObject* parent) : QObject(parent) {
	this->graph = nullptr;
	this->session = nullptr;
	this->status = TF_NewStatus();
	this->run_options = TF_NewBuffer();
	this->run_metadata = TF_NewBuffer();
	this->labels.clear();
	this->labels.reserve(10);

	connect(this, &ClassificationWorker::sig_classify, this, &ClassificationWorker::classify, Qt::QueuedConnection);

	this->data_queue[0].reserve(max_data_size);
	this->data_queue[1].reserve(max_data_size);
	this->data_queue_index = 0;
}

ClassificationWorker::~ClassificationWorker() {
	if (this->session) {
		TF_CloseSession(this->session, this->status);
		TF_DeleteSession(this->session, this->status);
	}
	if (this->graph) {
		TF_DeleteGraph(this->graph);
	}
	TF_DeleteStatus(this->status);
	TF_DeleteBuffer(this->run_options);
	TF_DeleteBuffer(this->run_metadata);
}

void ClassificationWorker::init(std::string model_path, std::string label_path) {
	const char* tags[] = {"serve"};

	this->graph = TF_NewGraph();
	if (!this->graph) {
		qDebug() << "Failed to create graph";
		return;
	}
	TF_SessionOptions* session_options = TF_NewSessionOptions();
	if (!session_options) {
		qDebug() << "Failed to create session options";
		return;
	}
	TF_SetConfig(session_options, nullptr, 0, this->status);
	if (TF_GetCode(this->status) != TF_OK) {
		qDebug() << "Failed to set session options: " << TF_Message(this->status);
		return;
	}
	this->session = TF_LoadSessionFromSavedModel(session_options, this->run_options, model_path.c_str(), tags, 1,
												 this->graph, this->run_metadata, this->status);
	if (TF_GetCode(this->status) != TF_OK) {
		qDebug() << "Failed to load model: " << TF_Message(this->status);
		TF_DeleteSessionOptions(session_options);
		return;
	}

	std::ifstream label_file(label_path);
	if (!label_file.is_open()) {
		qDebug() << "Failed to open label file";
		return;
	}
	std::string line;
	while (std::getline(label_file, line)) {
		this->labels.push_back(QString::fromStdString(line));
	}
	label_file.close();
}

void ClassificationWorker::addData(QString key, float X, float Y, float Z) {
	bool is_avaliable = __atomic_load_n(&this->data_queue_available[this->data_queue_index], std::memory_order_acquire);
	if (!is_avaliable) {
		return;
	}
	ClassificationDataPoint data = {X, Y, Z};
	if (this->data_queue[this->data_queue_index].contains(key)) {
		this->data_queue[this->data_queue_index][key].push_back(data);
	} else {
		this->data_queue[this->data_queue_index][key] = {data};
	}
	if (this->data_queue[this->data_queue_index][key].size() >= this->max_data_size) {
		emit sig_classify(this->data_queue_index);
		this->data_queue_index = (this->data_queue_index + 1) % 2;
	}
}

void ClassificationWorker::classify(int index) {
	if (!this->graph || !this->session) {
		qDebug() << "Graph or session is not initialized";
		return;
	}
	__atomic_store_n(&this->data_queue_available[index], false, std::memory_order_release);
	if (this->data_queue[index].size() == 0) {
		qDebug() << "No data to classify";
		return;
	}
	TF_Output input_op = {TF_GraphOperationByName(this->graph, "serving_default_tb_input"), 0};
	TF_Output output_op = {TF_GraphOperationByName(this->graph, "StatefulPartitionedCall"), 0};
	if (input_op.oper == nullptr || output_op.oper == nullptr) {
		qDebug() << "Failed to get input/output operations";
		return;
	}

	// prepare input data
	const int num_sensors = 5;
	const int num_features = 3;
	if (this->data_queue[index].size() != num_sensors) {
		qDebug() << "Invalid sensor number: " << this->data_queue[index].size() << ", expected: " << num_sensors;
		return;
	}
	int timesteps = INT_MAX;
	for (auto it : this->data_queue[index]) {
		timesteps = std::min(timesteps, (int)it.size());
	}
	if (timesteps == INT_MAX) {
		qDebug() << "No data to classify";
		return;
	}
	// qDebug() << "timesteps: " << timesteps;
	const int64_t dims[] = {1, timesteps, num_sensors, num_features};
	TF_Tensor* input_tensor =
		TF_AllocateTensor(TF_FLOAT, dims, 4, sizeof(float) * timesteps * num_sensors * num_features);
	float* input_data = static_cast<float*>(TF_TensorData(input_tensor));
	auto keys = this->data_queue[index].keys();
	std::sort(keys.begin(), keys.end());
	for (int i = 0; i < num_sensors; i++) {
		auto key = keys[i];
		for (int j = 0; j < timesteps; j++) {
			input_data[i * timesteps * num_features + j * num_features + 0] = this->data_queue[index][key][j].X;
			input_data[i * timesteps * num_features + j * num_features + 1] = this->data_queue[index][key][j].Y;
			input_data[i * timesteps * num_features + j * num_features + 2] = this->data_queue[index][key][j].Z;
		}
	}
	TF_Tensor* output_tensor = TF_AllocateTensor(TF_FLOAT, nullptr, 0, sizeof(float) * this->labels.size());
	auto start_time = std::chrono::high_resolution_clock::now();
	TF_SessionRun(this->session, this->run_options, &input_op, &input_tensor, 1, &output_op, &output_tensor, 1, nullptr,
				  0, nullptr, this->status);
	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
	// qDebug() << "Classification time: " << duration << " ms";
	if (TF_GetCode(this->status) != TF_OK) {
		qDebug() << "Failed to run session: " << TF_Message(this->status);
		TF_DeleteTensor(input_tensor);
		TF_DeleteTensor(output_tensor);
		return;
	}

	// get output
	std::vector<float> output_data(this->labels.size());
	memcpy(output_data.data(), TF_TensorData(output_tensor), this->labels.size() * sizeof(float));
	QString result = this->labels[argmax(output_data)];
	emit sig_classificationResult(result);
	// qDebug() << "Classification result: " << result;

	TF_DeleteTensor(input_tensor);
	TF_DeleteTensor(output_tensor);

	// clear data queue
	this->data_queue[index].clear();
	__atomic_store_n(&this->data_queue_available[index], true, std::memory_order_release);
}

void ClassificationWorker::classifyCurrentSlot() {
	if (this->data_queue[this->data_queue_index].size() > 0
		&& this->data_queue[this->data_queue_index].begin()->size() >= 5) {
		emit sig_classify(this->data_queue_index);
		this->data_queue_index = (this->data_queue_index + 1) % 2;
	}
}
