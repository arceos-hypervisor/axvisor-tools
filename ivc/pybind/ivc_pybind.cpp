#include <ivc/ulib.h>

#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <pybind11/pybind11.h>
#include <string>
#include <vector>

namespace py = pybind11;

class ManagerPointerManager
{
  public:
	ManagerPointerManager()
	{
		manager = ivc_open_manager();
		if (!manager)
		{
			py::set_error(PyExc_RuntimeError, "Failed to open IVC manager");
			throw ::std::exception();
		}
	}

	~ManagerPointerManager()
	{
		if (manager)
			(void)ivc_close_manager(manager);
	}

	ivc_manager_p get_manager() const { return manager; }

  private:
	ivc_manager_p manager;
};

typedef ::std::shared_ptr<ManagerPointerManager> ManagerPtr;

class Manager
{
  public:
	Manager() { manager = ::std::make_shared<ManagerPointerManager>(); }

	~Manager() {}

	ManagerPtr copy_manager_ptr() const { return manager; }

  private:
	ManagerPtr manager;
};

class Subscriber
{
  public:
	Subscriber(const Manager &mgr, uint64_t publisher_id, uint64_t channel_key)
	{
		manager_ptr = mgr.copy_manager_ptr();
		subscriber = ivc_subscribe(
			manager_ptr->get_manager(), publisher_id, channel_key);
		if (!subscriber)
		{
			py::set_error(PyExc_RuntimeError, "Failed to subscribe to channel");
			throw ::std::exception();
		}
	}

	~Subscriber() { (void)ivc_unsubscribe(subscriber); }

	py::bytes recv(size_t max_bytes)
	{
		if (max_bytes == 0)
			throw py::value_error("max_bytes must be greater than zero");
		if (max_bytes > static_cast<size_t>(std::numeric_limits<int>::max()))
			throw py::value_error("max_bytes must not exceed INT_MAX");

		std::vector<char> buffer(max_bytes);
		int bytes_received =
			ivc_subscriber_recv(subscriber, buffer.data(), max_bytes);
		if (bytes_received < 0)
		{
			py::set_error(
				PyExc_RuntimeError, "Failed to receive on subscriber device");
			throw ::std::exception();
		}
		else if (bytes_received == 0)
		{
			return py::bytes();
		}

		buffer.resize(bytes_received);
		return py::bytes(buffer.data(), bytes_received);
	}

	int send(const py::bytes &data)
	{
		std::string data_str = data;
		int bytes_sent =
			ivc_subscriber_send(subscriber, data_str.data(), data_str.size());
		if (bytes_sent < 0)
		{
			py::set_error(
				PyExc_RuntimeError, "Failed to send on subscriber device");
			throw ::std::exception();
		}
		return bytes_sent;
	}

  private:
	ManagerPtr manager_ptr;
	ivc_subscriber_p subscriber;
};

class Publisher
{
  public:
	Publisher(const Manager &mgr, uint64_t channel_key, uint64_t channel_size)
	{
		manager_ptr = mgr.copy_manager_ptr();
		publisher =
			ivc_publish(manager_ptr->get_manager(), channel_key, channel_size);
		if (!publisher)
		{
			py::set_error(PyExc_RuntimeError, "Failed to publish channel");
			throw ::std::exception();
		}
	}

	~Publisher() { (void)ivc_unpublish(publisher); }

	py::bytes recv(size_t max_bytes)
	{
		if (max_bytes == 0)
			throw py::value_error("max_bytes must be greater than zero");
		if (max_bytes > static_cast<size_t>(std::numeric_limits<int>::max()))
			throw py::value_error("max_bytes must not exceed INT_MAX");

		std::vector<char> buffer(max_bytes);
		int bytes_received =
			ivc_publisher_recv(publisher, buffer.data(), max_bytes);
		if (bytes_received < 0)
		{
			py::set_error(
				PyExc_RuntimeError, "Failed to receive on publisher device");
			throw ::std::exception();
		}
		else if (bytes_received == 0)
		{
			return py::bytes();
		}

		buffer.resize(bytes_received);
		return py::bytes(buffer.data(), bytes_received);
	}

	int send(const py::bytes &data)
	{
		std::string data_str = data;
		int bytes_sent =
			ivc_publisher_send(publisher, data_str.data(), data_str.size());
		if (bytes_sent < 0)
		{
			py::set_error(
				PyExc_RuntimeError, "Failed to send on publisher device");
			throw ::std::exception();
		}
		return bytes_sent;
	}

  private:
	ManagerPtr manager_ptr;
	ivc_publisher_p publisher;
};

PYBIND11_MODULE(ivcpy, m)
{
	m.doc() = "Axvisor IVC Library Python Bindings";

	py::class_<Manager>(m, "Manager").def(py::init<>());

	py::class_<Subscriber>(m, "Subscriber")
		.def(py::init<const Manager &, uint64_t, uint64_t>())
		.def("recv", &Subscriber::recv)
		.def("send", &Subscriber::send);

	py::class_<Publisher>(m, "Publisher")
		.def(py::init<const Manager &, uint64_t, uint64_t>())
		.def("recv", &Publisher::recv)
		.def("send", &Publisher::send);
}
