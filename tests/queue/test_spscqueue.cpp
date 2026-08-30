#include <catch2/catch_test_macros.hpp>
#include <queue/SpscQueue.hpp>
#include <thread>
#include <vector>

using namespace surma::queue;

TEST_CASE("queue is empty on construction", "[unit][spsc]")
{
	SpscQueue<int, 8> q;
	REQUIRE(q.empty());
	REQUIRE_FALSE(q.full());
	REQUIRE(q.size() == 0);
}

TEST_CASE("push and pop single item", "[unit][spsc]")
{
	SpscQueue<int, 8> q;

	REQUIRE(q.push(42));

	auto item = q.pop();
	REQUIRE(item.has_value());
	REQUIRE(*item == 42);
	REQUIRE(q.empty());
}

TEST_CASE("pop from empty queue returns nullopt", "[unit][spsc]")
{
	SpscQueue<int, 8> q;
	REQUIRE_FALSE(q.pop().has_value());
}

TEST_CASE("push to full queue returns false", "[unit][spsc]")
{
	SpscQueue<int, 4> q;

	REQUIRE(q.push(1));
	REQUIRE(q.push(2));
	REQUIRE(q.push(3));
	REQUIRE_FALSE(q.push(4));
	REQUIRE(q.full());
}

TEST_CASE("capacity is correct", "[unit][spsc]")
{
	REQUIRE(SpscQueue<int, 8>::capacity() == 8);
	REQUIRE(SpscQueue<int, 1024>::capacity() == 1024);
}

TEST_CASE("fifo ordering is preserved", "[unit][spsc]")
{
	SpscQueue<int, 16> q;

	for (int i = 0; i < 10; i++)
		REQUIRE(q.push(i));

	for (int i = 0; i < 10; i++)
	{
		auto item = q.pop();
		REQUIRE(item.has_value());
		REQUIRE(*item == i);
	}
}

TEST_CASE("queue wraps around correctly", "[unit][spsc]")
{
	SpscQueue<int, 4> q;

	for (int round = 0; round < 2; round++)
	{
		REQUIRE(q.push(round * 10 + 1));
		REQUIRE(q.push(round * 10 + 2));
		REQUIRE(q.push(round * 10 + 3));

		REQUIRE(q.pop().value() == round * 10 + 1);
		REQUIRE(q.pop().value() == round * 10 + 2);
		REQUIRE(q.pop().value() == round * 10 + 3);
		REQUIRE(q.empty());
	}
}

TEST_CASE("concurrent producer consumer", "[unit][spsc]")
{
	constexpr int ITEM_COUNT = 100000;
	SpscQueue<int, 1024> q;

	std::vector<int> received;
	received.reserve(ITEM_COUNT);

	std::thread producer([&]() {
		for (int i = 0; i < ITEM_COUNT; i++)
		{
			while (!q.push(i))
				;
		}
	});

	std::thread consumer([&]() {
		int count = 0;
		while (count < ITEM_COUNT)
		{
			auto item = q.pop();
			if (item.has_value())
			{
				received.push_back(*item);
				count++;
			}
		}
	});

	producer.join();
	consumer.join();

	REQUIRE(received.size() == ITEM_COUNT);
	for (int i = 0; i < ITEM_COUNT; i++)
		REQUIRE(received[i] == i);
}
