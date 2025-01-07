#ifndef _NETWORK_HPP
#define _NETWORK_HPP

#include <QObject>
#include <tensorflow/c/c_api.h>

class Network : public QObject {
	Q_OBJECT
public:
	Network();
	~Network();

	void test_tf();
};

#endif // _NETWORK_HPP
