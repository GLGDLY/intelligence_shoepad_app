#ifndef _DATA_CONTAINER_HPP
#define _DATA_CONTAINER_HPP

#include <QtTypes>
#include <stdint.h>
#include <time.h>
#include <tuple>
#include <utility>



class DataContainer {
public:
	DataContainer(int capacity = 1);
	~DataContainer();

	void append(qint64 timestamp, int16_t X, int16_t Y, int16_t Z);
	void clear();

	int size() const;	  // return current number of data
	int capacity() const; // return maximum number of data

	std::pair<qint64, std::tuple<int16_t, int16_t, int16_t>> at(int index) const;
	std::pair<qint64, std::tuple<int16_t, int16_t, int16_t>> operator[](int index) const;

	// iterator
	class iterator {
	public:
		iterator(DataContainer* container, int index);
		iterator(const iterator& other);
		~iterator();

		iterator& operator++();
		iterator operator++(int);
		std::pair<qint64, std::tuple<int16_t, int16_t, int16_t>> operator*() const;
		bool operator==(const iterator& other) const;
		bool operator!=(const iterator& other) const;

	private:
		DataContainer* container;
		int index;
	};

	iterator begin();
	iterator end();

private:
	int _capacity;
	int head, tail;
	time_t* timestamps;
	int16_t *data_X, *data_Y, *data_Z;
};

#endif // _DATA_CONTAINER_HPP