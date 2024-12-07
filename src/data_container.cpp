#include "data_container.hpp"

#include <stdexcept>

DataContainer::DataContainer(int capacity)
	: _capacity(capacity)
	, head(0)
	, tail(0)
	, timestamps(new time_t[capacity])
	, data_X(new int16_t[capacity])
	, data_Y(new int16_t[capacity])
	, data_Z(new int16_t[capacity]) {}

DataContainer::~DataContainer() {
	delete[] timestamps;
	delete[] data_X;
	delete[] data_Y;
	delete[] data_Z;
}

void DataContainer::append(time_t timestamp, int16_t X, int16_t Y, int16_t Z) {
	timestamps[tail] = timestamp;
	this->data_X[tail] = X;
	this->data_Y[tail] = Y;
	this->data_Z[tail] = Z;
	tail = (tail + 1) % _capacity;
}

void DataContainer::clear() { head = tail = 0; }

int DataContainer::size() const { return (tail - head + _capacity) % _capacity; }

int DataContainer::capacity() const { return _capacity; }

std::pair<time_t, std::tuple<int16_t, int16_t, int16_t>> DataContainer::at(int index) const {
	if (index < 0 || index >= this->size()) {
		throw std::out_of_range("Index out of range");
	}

	const int target_i = (head + index) % _capacity;
	return std::make_pair(timestamps[target_i], std::make_tuple(data_X[target_i], data_Y[target_i], data_Z[target_i]));
}

std::pair<time_t, std::tuple<int16_t, int16_t, int16_t>> DataContainer::operator[](int index) const {
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

std::pair<time_t, std::tuple<int16_t, int16_t, int16_t>> DataContainer::iterator::operator*() const {
	return std::make_pair(
		container->timestamps[index],
		std::make_tuple(container->data_X[index], container->data_Y[index], container->data_Z[index]));
}

bool DataContainer::iterator::operator==(const iterator& other) const { return index == other.index; }

bool DataContainer::iterator::operator!=(const iterator& other) const { return index != other.index; }

DataContainer::iterator DataContainer::begin() { return iterator(this, head); }

DataContainer::iterator DataContainer::end() { return iterator(this, tail); }
