#include "data_container.hpp"

#include <QDebug>
#include <stdexcept>

DataContainer::DataContainer(int capacity)
	: _capacity(capacity)
	, head(0)
	, tail(0)
	, timestamps(new qint64[capacity])
	, data_X(new int16_t[capacity])
	, data_Y(new int16_t[capacity])
	, data_Z(new int16_t[capacity])
	, lock(QReadWriteLock::NonRecursive) {}

DataContainer::~DataContainer() {
	delete[] timestamps;
	delete[] data_X;
	delete[] data_Y;
	delete[] data_Z;
}

void DataContainer::append(qint64 timestamp, int16_t X, int16_t Y, int16_t Z) {
	this->lock.lockForWrite();
	// qDebug() << "Appending data: " << timestamp << " " << X << " " << Y << " " << Z;
	// qDebug() << "Head: " << head << " Tail: " << tail;
	this->timestamps[tail] = timestamp;
	this->data_X[tail] = X;
	this->data_Y[tail] = Y;
	this->data_Z[tail] = Z;
	tail = (tail + 1) % _capacity;
	if (tail == head) {
		head = (head + 1) % _capacity;
	}
	this->lock.unlock();
}

void DataContainer::clear() { head = tail = 0; }

int DataContainer::size() const { return (tail - head + _capacity) % _capacity; }

int DataContainer::capacity() const { return _capacity; }

std::pair<qint64, std::tuple<int16_t, int16_t, int16_t>> DataContainer::at(int index) const {
	if (index < 0 || index >= this->size()) {
		qDebug() << "Index out of range";
		throw std::out_of_range("Index out of range");
	}

	const int target_i = (head + index) % _capacity;
	return std::make_pair(timestamps[target_i], std::make_tuple(data_X[target_i], data_Y[target_i], data_Z[target_i]));
}

std::pair<qint64, std::tuple<int16_t, int16_t, int16_t>> DataContainer::operator[](int index) const {
	return this->at(index);
}

DataContainer::iterator::iterator(DataContainer* container, int index) : container(container), index(index) {}

DataContainer::iterator::iterator(const iterator& other) : container(other.container), index(other.index) {}

DataContainer::iterator::~iterator() {}

DataContainer::iterator& DataContainer::iterator::operator++() {
	index = (index + 1) % container->_capacity;
	return *this;
}

DataContainer::iterator DataContainer::iterator::operator++(int) {
	iterator tmp = *this;
	++(*this);
	return tmp;
}

std::pair<qint64, std::tuple<int16_t, int16_t, int16_t>> DataContainer::iterator::operator*() const {
	qDebug() << "index: " << index << " head: " << container->head << " tail: " << container->tail;
	qDebug() << "timestamps: " << container->timestamps[index] << " data: " << container->data_X[index] << " "
			 << container->data_Y[index] << " " << container->data_Z[index];
	return std::make_pair(
		container->timestamps[index],
		std::make_tuple(container->data_X[index], container->data_Y[index], container->data_Z[index]));
}

bool DataContainer::iterator::operator==(const iterator& other) const { return index == other.index; }

bool DataContainer::iterator::operator!=(const iterator& other) const { return index != other.index; }

DataContainer::iterator DataContainer::begin() { return iterator(this, head); }

DataContainer::iterator DataContainer::end() { return iterator(this, tail); }
