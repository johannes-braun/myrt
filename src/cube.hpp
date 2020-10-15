#pragma once

#include <array>

namespace cube
{
	constexpr float vertices[][3] = {
		-1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f
	};

	constexpr float normals[][3] = {
		 0.0f,  0.0f,  1.0f,    0.0f,  0.0f,  1.0f,    0.0f,  0.0f,  1.0f,    0.0f,  0.0f,  1.0f,
		-1.0f,  0.0f,  0.0f,   -1.0f,  0.0f,  0.0f,   -1.0f,  0.0f,  0.0f,   -1.0f,  0.0f,  0.0f,
		 1.0f,  0.0f,  0.0f,    1.0f,  0.0f,  0.0f,    1.0f,  0.0f,  0.0f,    1.0f,  0.0f,  0.0f,
		 0.0f,  1.0f,  0.0f,    0.0f,  1.0f,  0.0f,    0.0f,  1.0f,  0.0f,    0.0f,  1.0f,  0.0f,
		 0.0f, -1.0f,  0.0f,    0.0f, -1.0f,  0.0f,    0.0f, -1.0f,  0.0f,    0.0f, -1.0f,  0.0f,
		 0.0f,  0.0f, -1.0f,    0.0f,  0.0f, -1.0f,    0.0f,  0.0f, -1.0f,    0.0f,  0.0f, -1.0f
	};

	constexpr float uvs[][3] = {
		0.0f,  1.0f,    0.0f,  0.0f,    1.0f,  0.0f,    1.0f,  1.0f,
		0.0f,  1.0f,    0.0f,  0.0f,    1.0f,  0.0f,    1.0f,  1.0f,
		0.0f,  1.0f,    0.0f,  0.0f,    1.0f,  0.0f,    1.0f,  1.0f,
		0.0f,  1.0f,    0.0f,  0.0f,    1.0f,  0.0f,    1.0f,  1.0f,
		0.0f,  1.0f,    0.0f,  0.0f,    1.0f,  0.0f,    1.0f,  1.0f,
		0.0f,  1.0f,    0.0f,  0.0f,    1.0f,  0.0f,    1.0f,  1.0f
	};

	constexpr size_t num_points = 24;
	constexpr size_t num_indices = 36;

	constexpr auto indices = []() {
		std::array<unsigned, 36> ind{};
		size_t insert_at = 0;
		for (unsigned i = 0; i < 6; i++)
		{
			ind[insert_at++] = (i * 4 + 0);
			ind[insert_at++] = (i * 4 + 1);
			ind[insert_at++] = (i * 4 + 2);
			ind[insert_at++] = (i * 4 + 2);
			ind[insert_at++] = (i * 4 + 3);
			ind[insert_at++] = (i * 4 + 0);
		}
		return ind;
	}();

}