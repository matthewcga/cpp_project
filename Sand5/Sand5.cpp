#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <map>
#include <cstdlib>
#include <chrono>
#include <thread>

using namespace sf;
using namespace std;

constexpr int SIZE = 480;
constexpr int BRUSH = 1;
unsigned AGE = 1;

Text text_title, text_help;

#pragma region Elements

struct particle
{
	virtual ~particle() = default;

	particle()
	{
		age = AGE;
		velocity = 0.0f;
		horizontal = 0.0f;
		direction = 0;
		low_density = false;
		toxic_chance = 200;
	}

	RectangleShape shape;
	unsigned age;
	float velocity, horizontal;
	int direction, toxic_chance;
	bool low_density;

	virtual void init(Vector2f pos) = 0;

	virtual void update(int y, int x) = 0;

	void swap_particles(int y, int x, int new_y, int new_x) const;
};

struct sand final : particle
{
	void init(Vector2f pos) override;

	void update(int y, int x) override;
};

struct stone final : particle
{
	void init(Vector2f pos) override;

	void update(int y, int x) override;
};

struct water final : particle
{
	void init(Vector2f pos) override;

	void update(int y, int x) override;
};

struct toxic final : particle
{
	void init(Vector2f pos) override;

	void update(int y, int x) override;
};

#pragma endregion

particle* particles[SIZE][SIZE];

#pragma region Misc

bool random_bool()
{
	return rand() > (RAND_MAX / 2);
}

int random_int(const int max, const int min = 1)
{
	return (rand() % (std::max(1, max))) + min;
}

int map_velocity(const int v)
{
	return random_int(min(10, v / 50) + 2);
}

bool is_in_range(const int y, const int x)
{
	return y >= 0 && y < SIZE&& x >= 0 && x < SIZE;
}

bool is_cell_empty(const int y, const int x)
{
	return particles[y][x] == nullptr;
}

void erase(const int y, const int x)
{
	free(particles[y][x]);
	particles[y][x] = nullptr;
}

Color get_rand_color(const int r, const int g, const int b)
{
	return {
		static_cast<Uint8>(r + random_int(80)),
		static_cast<Uint8>(g + random_int(80)),
		static_cast<Uint8>(b + random_int(80))
	};
}

#pragma endregion

#pragma region Particle

void particle::swap_particles(const int y, const int x, const int new_y, const int new_x) const
{
	particles[y][x]->shape.setPosition(Vector2f(static_cast<float>(new_x), static_cast<float>(new_y)));
	swap(particles[y][x],
		particles[new_y][new_x]);
}

#pragma endregion

#pragma region Sand

void add_sand(const Vector2f pos)
{
	const int y = static_cast<int>(pos.y), x = static_cast<int>(pos.x);
	particles[y][x] = new sand();
	particles[y][x]->init(pos);
}

void sand::init(const Vector2f pos)
{
	shape.setSize(Vector2f(1, 1));
	shape.setFillColor(get_rand_color(150, 150, 40));
	shape.setOrigin(shape.getSize());
	shape.setPosition(pos);
	toxic_chance = 10;
}

void sand::update(const int y, const int x)
{
	if (age != AGE) return;

	age++;
	const int offset = random_bool() ? 1 : -1;
	int biggest;

	if (is_in_range(y + 1, x) && (is_cell_empty(y + 1, x) || particles[y + 1][x]->low_density))
	{
		biggest = 1;
		velocity += 0.05f;
		const int v = static_cast<int>(velocity);

		if (v > 1)
			for (int i = 2; i <= v; i++)
			{
				if (is_in_range(y + i, x) && (is_cell_empty(y + i, x) || particles[y + i][x]->low_density))
					biggest = i;
				else break;
			}

		swap_particles(y, x, y + biggest, x);
	}
	else if (is_in_range(y + 1, x + offset) && (is_cell_empty(y + 1, x + offset) || particles[y + 1][x + offset]->
		low_density))
	{
		swap_particles(y, x, y + 1, x + offset);
		direction = offset;
	}
	else if (is_in_range(y + 1, x - offset) && (is_cell_empty(y + 1, x - offset) || particles[y + 1][x - offset]->
		low_density))
	{
		swap_particles(y, x, y + 1, x - offset);
		direction = offset;
	}
	else
	{
		biggest = 0;
		const int v = static_cast<int>(velocity) * 4;
		if (v <= 1) return;
		if (direction == 0) direction = offset;

		for (int i = direction; i != v * direction; i += direction)
			if (is_in_range(y, x + i) && (is_cell_empty(y, x + i) || particles[y][x + i]->low_density))
				biggest = i;
			else break;

		if (biggest)
			swap_particles(y, x, y, x + biggest);

		velocity /= 2.0f;
	}
}

#pragma endregion

#pragma region Stone

void add_stone(const Vector2f pos)
{
	const int y = static_cast<int>(pos.y), x = static_cast<int>(pos.x);
	particles[y][x] = new stone();
	particles[y][x]->init(pos);
}

void stone::init(const Vector2f pos)
{
	shape.setSize(Vector2f(1, 1));
	shape.setFillColor(get_rand_color(120, 120, 120));
	shape.setOrigin(shape.getSize());
	shape.setPosition(pos);
	toxic_chance = 3;
}

void stone::update(int y, int x)
{
}

#pragma endregion

#pragma region Water

void add_water(const Vector2f pos)
{
	const int y = static_cast<int>(pos.y), x = static_cast<int>(pos.x);
	particles[y][x] = new water();
	particles[y][x]->init(pos);
}

void water::init(const Vector2f pos)
{
	shape.setSize(Vector2f(1, 1));
	shape.setFillColor(get_rand_color(60, 100, 150));
	shape.setOrigin(shape.getSize());
	shape.setPosition(pos);
	low_density = true;
	toxic_chance = 10;
}

void water::update(const int y, const int x)
{
	if (age != AGE) return;

	age++;
	const int offset = random_bool() ? 1 : -1;
	int biggest_x;
	const int v = static_cast<int>(velocity) * 8, h = static_cast<int>(horizontal);

	if (is_in_range(y + 1, x) && is_cell_empty(y + 1, x))
	{
		int biggest = 1;
		biggest_x = 0;
		velocity += 0.05f;

		if (v > 1)
			for (int i = 2; i <= v; i++)
			{
				if (is_in_range(y + i, x) && is_cell_empty(y + i, x))
					biggest = i;
				else break;
			}

		if (h > 0 && direction)
			for (int i = 1; i < h * direction; i += direction)
			{
				if (is_in_range(y + biggest, x + i) && is_cell_empty(y + biggest, x + i))
					biggest_x = i;
				else break;
			}

		swap_particles(y, x, y + biggest, x + biggest_x * direction);
	}
	else if (h > 0 && direction && is_in_range(y, x + direction) && is_cell_empty(y, x + direction))
	{
		biggest_x = 1;

		for (int i = 2; i <= h * direction; i += direction)
			if (is_in_range(y, x + i) && is_cell_empty(y, x + i))
				biggest_x = i;
			else break;

		swap_particles(y, x, y, x + biggest_x * direction);
	}
	else if (is_in_range(y + 1, x + offset) && (is_cell_empty(y + 1, x + offset)))
	{
		swap_particles(y, x, y + 1, x + offset);
		direction = offset;
		horizontal = max(horizontal + 0.5f, 0.5f);
	}
	else if (is_in_range(y + 1, x - offset) && (is_cell_empty(y + 1, x - offset)))
	{
		swap_particles(y, x, y + 1, x - offset);
		direction = offset;
		horizontal = max(horizontal + 0.5f, 0.5f);
	}
	else
	{
		if (is_in_range(y, x + offset) && (is_cell_empty(y, x + offset)))
		{
			swap_particles(y, x, y, x + offset);
			direction = offset;
		}
		else if (is_in_range(y, x - offset) && (is_cell_empty(y, x - offset)))
		{
			swap_particles(y, x, y, x - offset);
			direction = offset;
		}

		velocity /= 2.0f;
		horizontal -= 0.03f;
	}
}

#pragma endregion

#pragma region Toxic

void add_toxic(const Vector2f pos)
{
	const int y = static_cast<int>(pos.y), x = static_cast<int>(pos.x);
	particles[y][x] = new toxic();
	particles[y][x]->init(pos);
}

void toxic::init(const Vector2f pos)
{
	shape.setSize(Vector2f(1, 1));
	shape.setFillColor(get_rand_color(30, 150, 30));
	shape.setOrigin(shape.getSize());
	shape.setPosition(pos);
	toxic_chance = 0;
}

void apply_toxic_effect(const int y, const int x)
{
	erase(y, x);
	add_toxic(Vector2f(static_cast<float>(x), static_cast<float>(y)));
	particles[y][x]->update(y, x);
}

void toxic::update(const int y, const int x)
{
	if (age != AGE) return;

	const int offset_x = x + (random_bool() ? 1 : -1), offset_y = y + (random_bool() ? 1 : -1);

	if (is_in_range(offset_y, offset_x))
		if (!is_cell_empty(offset_y, offset_x))
			if (particles[offset_y][offset_x]->toxic_chance > random_int(100))
			{
				//cout << y << " + " << 1 << "   :   " << x << " + " << 0 << "\tNOT EMPTY\n";
				apply_toxic_effect(offset_y, offset_x);
			}
	// && particles[offset_y][offset_x]->toxic_chance < random_int(100)
	//if (is_in_range(y + 1, x) && !is_cell_empty(y + 1, x)) apply_toxic_effect(y + 1, x);


	age++;
	const int offset = random_bool() ? 1 : -1;
	int biggest;

	if (is_in_range(y + 1, x) && (is_cell_empty(y + 1, x) || particles[y + 1][x]->low_density))
	{
		biggest = 1;
		velocity += 0.05f;
		const int v = static_cast<int>(velocity);

		if (v > 1)
			for (int i = 2; i <= v; i++)
			{
				if (is_in_range(y + i, x) && (is_cell_empty(y + i, x) || particles[y + i][x]->low_density))
					biggest = i;
				else break;
			}

		swap_particles(y, x, y + biggest, x);
	}
	else if (is_in_range(y + 1, x + offset) && (is_cell_empty(y + 1, x + offset) || particles[y + 1][x + offset]->
		low_density))
	{
		swap_particles(y, x, y + 1, x + offset);
		direction = offset;
	}
	else if (is_in_range(y + 1, x - offset) && (is_cell_empty(y + 1, x - offset) || particles[y + 1][x - offset]->
		low_density))
	{
		swap_particles(y, x, y + 1, x - offset);
		direction = offset;
	}
	else
	{
		biggest = 0;
		const int v = static_cast<int>(velocity) * 4;
		if (v <= 1) return;
		if (direction == 0) direction = offset;

		for (int i = direction; i != v * direction; i += direction)
			if (is_in_range(y, x + i) && (is_cell_empty(y, x + i) || particles[y][x + i]->low_density))
				biggest = i;
			else break;

		if (biggest)
			swap_particles(y, x, y, x + biggest);

		velocity /= 2.0f;
	}
}

#pragma endregion

#pragma region Main

int main()
{
	srand(NULL);

	RenderWindow window(VideoMode(SIZE, SIZE, 32), "Sand Sim", Style::Default);

	float text_height = 100.0f;

	Font font;
	font.loadFromFile("font.ttf");

	text_help.setString(
		"?: W water, S sand, C stone, T toxic, E eraser\n?: press: key + lmb to spawn element\n?: press: rmb + E to erase everything");
	text_help.setCharacterSize(10);
	text_help.setFillColor(Color::White);
	text_help.setFont(font);

	text_title.setString("SAND SIM");
	text_title.setCharacterSize(50);
	text_title.setFillColor(Color::White);
	text_title.setFont(font);

	while (window.isOpen())
	{
		if (Mouse::isButtonPressed(Mouse::Button::Left))
		{
			auto org_pos = Vector2f(Mouse::getPosition(window));
			for (int i = -BRUSH; i < BRUSH; i++)
				for (int j = -BRUSH; j < BRUSH; j++)
				{
					auto mouse_pos = Vector2f(org_pos.x + static_cast<float>(j), org_pos.y + static_cast<float>(i));
					if (is_in_range(static_cast<int>(mouse_pos.y), static_cast<int>(mouse_pos.x)))
					{
						if (is_cell_empty(static_cast<int>(mouse_pos.y), static_cast<int>(mouse_pos.x)))
						{
							if (Keyboard::isKeyPressed(Keyboard::S))
								add_sand(mouse_pos);
							else if (Keyboard::isKeyPressed(Keyboard::C))
								add_stone(mouse_pos);
							else if (Keyboard::isKeyPressed(Keyboard::W))
								add_water(mouse_pos);
							else if (Keyboard::isKeyPressed(Keyboard::T))
								add_toxic(mouse_pos);
						}
						else if (Keyboard::isKeyPressed(Keyboard::E))
							erase(static_cast<int>(mouse_pos.y), static_cast<int>(mouse_pos.x));
					}
				}
		}

		if (Mouse::isButtonPressed(Mouse::Button::Right) && Keyboard::isKeyPressed(Keyboard::E))
			for (int y = SIZE - 1; y; y--)
				for (int x = 0; x < SIZE; x++)
					if (!is_cell_empty(y, x))
						erase(y, x);

		text_help.setPosition(10.0f, text_height + 80);
		text_title.setPosition(30.0f, text_height);
		window.draw(text_title);
		window.draw(text_help);
		if (AGE > 2000 && AGE < 4000) text_height -= 0.08f;


		for (int y = SIZE - 1; y; y--)
			for (int x = 0; x < SIZE; x++)
				if (!is_cell_empty(y, x))
				{
					window.draw(particles[y][x]->shape);
					particles[y][x]->update(y, x);
				}

		window.display();
		Event event{};

		while (window.pollEvent(event))
			if (event.type == Event::Closed)
				window.close();

		window.clear(Color::Black);
		AGE++;
	}

	return 0;
}

#pragma endregion
