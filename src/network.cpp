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
	TF_Status* status = TF_NewStatus();
	TF_Code code = TF_GetCode(status);
	char const* code_s = boost::describe::enum_to_string(code, "Unknown");
	qDebug() << "TensorFlow version: " << TF_Version();
	qDebug() << "Status: " << code_s;
	if (code != TF_OK) {
		qDebug() << "Status error: " << TF_Message(status);
	}
	TF_DeleteStatus(status);
}