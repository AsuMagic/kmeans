#include <algorithm>
#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <array>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

struct Cluster; // forward def for point

struct Point
{
	float x, y;
	size_t cluster_id;
};

struct Cluster
{
	Point mean;
	size_t pop, id;
};

// Random
std::random_device rd;
std::mt19937 gen(rd());

// Rendering helpers
void add_point(sf::RenderTarget &target, std::vector<sf::Vertex>& points, Point& pt, float radius = 2.0f, sf::Color color = sf::Color::White)
{
	float x = pt.x * target.getSize().x, y = pt.y * target.getSize().y;
	points.emplace_back(sf::Vector2f{x - radius, y - radius}, color);
	points.emplace_back(sf::Vector2f{x + radius, y - radius}, color);
	points.emplace_back(sf::Vector2f{x + radius, y + radius}, color);
	points.emplace_back(sf::Vector2f{x - radius, y + radius}, color);
}

// K-means helpers
template<class T>
T sqr(const T& a)
{
	return a * a;
}

template<class T>
T distance(T x1, T y1, T x2, T y2)
{
	return std::sqrt(sqr(x2 - x1) + sqr(y2 - y1));
}

template<class D, class T>
sf::Vector2<D> window_to_normal(sf::Vector2<T> v, sf::RenderTarget &target)
{
	return {v.x / static_cast<D>(target.getSize().x), v.y / static_cast<D>(target.getSize().y)};
}

struct KMeans
{
	std::vector<Point> points;
	std::vector<Cluster> clusters;

	KMeans(size_t point_count, size_t cluster_count) :
		points(point_count),
		clusters(cluster_count)
	{
		randomize_points();

		for (size_t i = 0; i < clusters.size(); ++i)
		{
			clusters[i].mean = points[std::uniform_int_distribution<size_t>{0, points.size() - 1}(gen)];
			clusters[i].id = i;
		}
	}

	void randomize_points(std::vector<Point>::iterator a, std::vector<Point>::iterator b, Point min = {0.0f, 0.0f}, Point max = {1.0f, 1.0f})
	{
		for (; a != b; ++a)
		{
			*a = {std::uniform_real_distribution{min.x, max.x}(gen),
				  std::uniform_real_distribution{min.y, max.y}(gen)};
		}
	}

	void randomize_points()
	{
		randomize_points(points.begin(), points.end());
	}

	Cluster& find_closest_cluster(const Point &pt)
	{
		std::vector<float> dist(clusters.size());

		for (auto &c : clusters)
		{
			dist[c.id] = distance(c.mean.x, c.mean.y, pt.x, pt.y);
		}

		return *(clusters.begin() + std::distance(dist.begin(), std::min_element(dist.begin(), dist.end())));
	}

	void recalculate_clusters()
	{
		for (auto &pt : points)
		{
			pt.cluster_id = find_closest_cluster(pt).id;
		}

		// Reset means and population
		for (auto &c : clusters)
		{
			c.pop = 0;
			c.mean = {0.0f, 0.0f};
		}

		// Accumulate means, recalculate populations
		for (auto& pt : points)
		{
			auto &c = clusters[pt.cluster_id];
			c.mean = {c.mean.x + pt.x, c.mean.y + pt.y};
			++c.pop;
		}

		// Divide populated clusters means
		for (auto &c : clusters)
		{
			if (c.pop)
			{
				c.mean.x /= float(c.pop);
				c.mean.y /= float(c.pop);
			}
		}
	}
};

int main()
{
	sf::RenderWindow win{sf::VideoMode{900, 900}, "K-means"};
	win.setFramerateLimit(60);

	KMeans k{8000, 3};
	std::vector<sf::Vertex> points;

	auto erase_points_around = [&k](float x, float y) {
		k.points.erase(std::remove_if(k.points.begin(), k.points.end(), [x, y](const Point &pt) { return distance(x, y, pt.x, pt.y) < 0.03f; }), k.points.end());
	};

	const std::array<sf::Color, 7> colors{{ {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {0, 255, 255}, {255, 0, 255}, {255, 255, 0}, {255, 255, 255} }};

	// Info text
	sf::Font font;
	font.loadFromFile("DejaVuSans.ttf");

	sf::Text text;
	text.setCharacterSize(14);
	text.setFont(font);
	text.setPosition(16, 16);

	bool is_erasing = false;
	while (win.isOpen())
	{
		for (sf::Event ev; win.pollEvent(ev);)
		{
			if (ev.type == sf::Event::KeyPressed)
			{
				if (ev.key.code == sf::Keyboard::Space)
				{
					k.recalculate_clusters();
				}
				else if (ev.key.code == sf::Keyboard::Escape)
				{
					win.close();
				}
				else if (ev.key.code == sf::Keyboard::R)
				{
					k.randomize_points();
				}
				else if (ev.key.code == sf::Keyboard::Add)
				{
					constexpr int added = 128;
					k.points.resize(k.points.size() + added);
					k.randomize_points(k.points.end() - added, k.points.end());
				}
				else if (ev.key.code == sf::Keyboard::Subtract)
				{
					constexpr int removed = 128;
					k.points.resize(std::max(static_cast<int>(k.points.size()) - removed, 0));
				}
				else if (ev.key.code == sf::Keyboard::Up)
				{
					k.clusters.push_back({{0.0f, 0.0f}, 0, k.clusters.size()});
				}
				else if (ev.key.code == sf::Keyboard::Down)
				{
					if (!k.clusters.empty())
					{
						k.clusters.resize(k.clusters.size() - 1);
					}
				}
			}
			else if (ev.type == sf::Event::MouseButtonPressed)
			{
				if (ev.mouseButton.button == sf::Mouse::Left)
				{
					constexpr int added = 16;
					k.points.resize(k.points.size() + added);
					sf::Vector2f pos = window_to_normal<float>(sf::Mouse::getPosition(win), win);
					k.randomize_points(k.points.end() - added, k.points.end(), {pos.x - 0.02f, pos.y - 0.02f}, {pos.x + 0.02f, pos.y + 0.02f});
				}
				else if (ev.mouseButton.button == sf::Mouse::Right)
				{
					is_erasing = true;
					sf::Vector2f pos = window_to_normal<float>(sf::Mouse::getPosition(win), win);
					erase_points_around(pos.x, pos.y);
				}
			}
			else if (ev.type == sf::Event::MouseButtonReleased)
			{
				if (ev.mouseButton.button == sf::Mouse::Right)
				{
					is_erasing = false;
				}
			}
			else if (ev.type == sf::Event::MouseMoved && is_erasing)
			{
				sf::Vector2f pos = window_to_normal<float>(sf::Vector2i{ev.mouseMove.x, ev.mouseMove.y}, win);
				erase_points_around(pos.x, pos.y);
			}
			else if (ev.type == sf::Event::Closed)
			{
				win.close();
			}
			else if (ev.type == sf::Event::Resized)
			{
				win.setView(sf::View{{0.0f, 0.0f, static_cast<float>(ev.size.width), static_cast<float>(ev.size.height)}});
			}
		}

		points.clear();
		win.clear({25, 27, 29});

		// Compute points
		for (auto &pt : k.points)
		{
			add_point(win, points, pt, 4.0f, colors[(pt.cluster_id < k.clusters.size()) ? std::min(pt.cluster_id, colors.size() - 1) : colors.size() - 1]);
		}

		for (auto &cluster : k.clusters)
		{
			add_point(win, points, cluster.mean, 12.0f, {0, 0, 0});
			add_point(win, points, cluster.mean, 10.0f, colors[std::min(cluster.id, colors.size() - 1)]);
		}

		win.draw(points.data(), points.size(), sf::Quads);

		// Render cluster names
		for (size_t i = 0; i < k.clusters.size(); ++i)
		{
			auto &c = k.clusters[i];

			sf::Text name;
			name.setFont(font);
			name.setCharacterSize(14);
			name.setString(std::string{"#"} + std::to_string(i));
			name.setPosition(c.mean.x * win.getSize().x - name.getLocalBounds().width * 0.5f, c.mean.y * win.getSize().y - name.getLocalBounds().height * 0.5f);
			name.setOutlineColor({0, 0, 0, 200});
			name.setOutlineThickness(2.0f);

			win.draw(name);
		}

		std::string str =
			"Clusters: " + std::to_string(k.clusters.size()) +
			"\nPoints: " + std::to_string(k.points.size());

		for (size_t i = 0; i < k.clusters.size(); ++i)
		{
			Cluster& c = k.clusters[i];
			str += "\n\nCluster #" + std::to_string(i + 1) + ":"
				   "\n\tCentroid (" + std::to_string(c.mean.x) + ", " + std::to_string(c.mean.y) + ")"
				   "\n\tPopulation " + std::to_string(c.pop);
		}

		// Update info text
		text.setString(str);

		// Text frame
		sf::RectangleShape frame{{std::max(320.0f, text.getLocalBounds().width + 16), text.getLocalBounds().height + 16}};
		frame.setFillColor({0, 0, 0, 180});
		frame.setPosition(8, 8);
		win.draw(frame);

		win.draw(text);

		win.display();
	}
}
