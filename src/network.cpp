#include "network.hpp"

#include <QDebug>
#include <boost/describe.hpp>

BOOST_DESCRIBE_ENUM(TF_Code, TF_OK, TF_CANCELLED, TF_UNKNOWN, TF_INVALID_ARGUMENT, TF_DEADLINE_EXCEEDED, TF_NOT_FOUND,
					TF_ALREADY_EXISTS, TF_PERMISSION_DENIED, TF_UNAUTHENTICATED, TF_RESOURCE_EXHAUSTED,
					TF_FAILED_PRECONDITION, TF_ABORTED, TF_OUT_OF_RANGE, TF_UNIMPLEMENTED, TF_INTERNAL, TF_UNAVAILABLE,
					TF_DATA_LOSS)

Network::Network() {}

Network::~Network() {}

void Network::test_tf() {
	// https://github.com/RoyVII/Tensorflow_C_Example/blob/master/src/tf_functions.cpp
	qDebug() << "TensorFlow version: " << TF_Version();

	TF_Status* status = TF_NewStatus();
	TF_SessionOptions* session_opts = TF_NewSessionOptions();
	TF_Buffer* run_opts = NULL;
	TF_Graph* graph = TF_NewGraph();
	const char* tags = "serve";
	const char* model_path = R"(C:\Users\leung\Downloads\mnist_test_model)";
	TF_Session* session =
		TF_LoadSessionFromSavedModel(session_opts, run_opts, model_path, &tags, 1, graph, NULL, status);

	if (TF_GetCode(status) != TF_OK) {
		TF_Code code = TF_GetCode(status);
		char const* code_s = boost::describe::enum_to_string(code, "Unknown");
		qDebug() << "Error loading model: " << code_s << " - " << TF_Message(status);
		goto cleanup;
	}

	qDebug() << "Model loaded successfully";

cleanup:
	TF_DeleteSession(session, status);
	if (TF_GetCode(status) != TF_OK) {
		TF_Code code = TF_GetCode(status);
		char const* code_s = boost::describe::enum_to_string(code, "Unknown");
		qDebug() << "Error closing session: " << code_s << " - " << TF_Message(status);
	}
	if (run_opts) {
		TF_DeleteBuffer(run_opts);
	}
	TF_DeleteStatus(status);
	TF_DeleteSessionOptions(session_opts);
	TF_DeleteGraph(graph);
}