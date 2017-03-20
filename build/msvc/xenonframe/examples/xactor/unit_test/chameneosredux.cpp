#include <string>
#include <vector>
#include <iostream>
#include <future>

#include "actor.h"

static std::string spell(size_t n) {
	static const std::string numbers[] = { " zero", " one", " two", " three", " four", " five", " six", " seven", " eight", " nine" };
	auto next = numbers[n % 10];
	return n / 10 ? spell(n / 10) + next : next;
}

enum color { blue = 0, red, yellow };

static std::ostream& operator<<(std::ostream &s, const color &c) {
	static const std::string names[] = { "blue", "red", "yellow" };
	return s << names[c];
}

static color table[3][3] = {
	{ blue, yellow, red },
	{ yellow, red, blue },
	{ red, blue, yellow },
};

static void show_complements() {
	for (auto i : { blue, red, yellow })
		for (auto j : { blue, red, yellow })
			std::cout << i << " + " << j << " -> " << (table[i][j]) << std::endl;
}

static void print_header(const std::initializer_list<color>& colors) {
	std::cout << std::endl;
	for (auto i : colors)
		std::cout << " " << i;
	std::cout << std::endl;
}

using namespace actor;

struct stop {};

static void broker(size_t meetings_count) {
	for (auto i = 0u; i < meetings_count; ++i) {
		receive([](const handle& handle_left, color color_left) {
			receive([&](const handle& handle_right, color color_right) {
				handle_left.send(handle_right, color_right);
				handle_right.send(handle_left, color_left);
			});
		});
	}
}

static std::mutex output_mutex;

static void cleanup(size_t color_count) {
	size_t summary = 0ul;
	receive_while(color_count,
		[](const handle& other, color) {
		other.send(stop());
	},
		[&](size_t mismatch) {
		summary += mismatch;
		color_count--;
	}
	);
	std::lock_guard<std::mutex> lock(output_mutex);
	std::cout << spell(summary) << std::endl;
}

static void chameneos(color current, const handle& broker) {
	auto meetings = 0ul, met_self = 0ul;
	const auto self = actor::self();
	auto alive = true;
	broker.send(self, current);
	receive_while(alive,
		[&](const handle& other, color colour) {
		meetings++;
		current = table[current][colour];
		if (other == self)
			met_self++;
		broker.send(self, current);
	},
		[&](stop) {
		std::lock_guard<std::mutex> lock(output_mutex);
		std::cout << meetings << " " << spell(met_self) << std::endl;
		broker.send(meetings);
		alive = false;
	}
	);
}

static void run(const std::initializer_list<color>& colors, size_t count) {
	print_header(colors);
	for (auto color : colors)
		spawn(chameneos, color, self());
	broker(count);
	cleanup(colors.size());
	return;
}

int main(int argc, char ** argv) {
	std::vector<std::string> args(argv + 1, argv + argc);
	auto count = args.size() ? std::stoul(args.at(0)) : 10000ul;
	show_complements();
	run({ blue, red, yellow }, count);
	run({ blue, red, yellow, red, yellow, blue, red, yellow, red, blue }, count);
	std::cout << std::endl;
	return 0;
}