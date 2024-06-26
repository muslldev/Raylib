#include <raylib.h>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

const int CANVAS_WIDTH = 1200;
const int CANVAS_HEIGHT = 800;

struct Node
{
    long id;
    double lon;
    double lat;

    double x;
    double y;

    Node(long id, double lon, double lat) : id(id), lon(lon), lat(lat), x(0), y(0) {}
};

struct Edge
{
    long u;
    long v;

    double ux;
    double uy;

    double vx;
    double vy;

    double dist; // расстояние между u-v

    Edge(long u, long v) : u(u), v(v), dist(0) {}
};

double euclidean_dist(double x1, double y1, double x2, double y2)
{
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

std::vector<Node> read_nodes(std::string path)
{
    std::fstream csv(path);

    // skip csv header
    std::string header;
    std::getline(csv, header);

    std::vector<Node> nodes;
    // read data
    for (std::string line; std::getline(csv, line);)
    {
        std::stringstream lineStream(line);
        std::string cell;

        std::getline(lineStream, cell, ',');
        long id = std::stoll(cell);

        std::getline(lineStream, cell, ',');
        double lon = std::stod(cell);

        std::getline(lineStream, cell, ',');
        double lat = std::stod(cell);

        nodes.emplace_back(id, lon, lat);
    }

    return nodes;
}

std::vector<Edge> read_edges(std::string path)
{
    std::fstream csv(path);

    // skip csv header
    std::string header;
    std::getline(csv, header);

    std::vector<Edge> edges;

    for (std::string line; std::getline(csv, line);)
    {
        std::stringstream lineStream(line);
        std::string cell;

        std::getline(lineStream, cell, ',');
        long u = std::stoll(cell);

        std::getline(lineStream, cell, ',');
        long v = std::stoll(cell);

        edges.emplace_back(u, v);
    }

    return edges;
}

std::vector<long> dijkstra(const std::vector<Node> &nodes, const std::vector<Edge> &edges, long start, long end)
{
    std::unordered_map<long, std::size_t> node_id_to_pos;
    for (std::size_t i = 0; i < nodes.size(); i++)
    {
        node_id_to_pos[nodes[i].id] = i;
    }

    std::vector<double> dist(nodes.size(), std::numeric_limits<double>::max());
    std::vector<long> prev(nodes.size(), -1);
    std::vector<bool> visited(nodes.size(), false);

    dist[node_id_to_pos[start]] = 0;

    auto compareDist = [&](long a, long b)
    {
        return dist[node_id_to_pos[a]] > dist[node_id_to_pos[b]];
    };

    std::priority_queue<long, std::vector<long>, decltype(compareDist)> pq(compareDist);
    pq.push(start);

    while (!pq.empty())
    {
        long u = pq.top();
        pq.pop();

        if (u == end)
            break;

        if (visited[node_id_to_pos[u]])
            continue;

        visited[node_id_to_pos[u]] = true;

        for (const auto &edge : edges)
        {
            if (edge.u == u)
            {
                double alt = dist[node_id_to_pos[u]] + edge.dist;
                if (alt < dist[node_id_to_pos[edge.v]])
                {
                    dist[node_id_to_pos[edge.v]] = alt;
                    prev[node_id_to_pos[edge.v]] = u;
                    pq.push(edge.v);
                }
            }
        }
    }

    std::vector<long> path;
    for (long at = end; at != -1; at = prev[node_id_to_pos[at]])
        path.push_back(at);

    std::reverse(path.begin(), path.end());

    return path;
}

int main()
{
    auto nodes = read_nodes("omsk/nodes.csv");
    auto edges = read_edges("omsk/edges.csv");

    std::unordered_map<long, std::size_t> node_id_to_pos;
    for (std::size_t i = 0; i < nodes.size(); i++)
    {
        auto &node = nodes[i];
        node_id_to_pos[node.id] = i;
    }

    std::cout << nodes.size() << std::endl;
    std::cout << edges.size() << std::endl;

    double min_lon = nodes[0].lon;
    double max_lon = nodes[0].lon;
    double min_lat = nodes[0].lat;
    double max_lat = nodes[0].lat;

    for (auto &node : nodes)
    {
        if (min_lat > node.lat)
        {
            min_lat = node.lat;
        }
        if (min_lon > node.lon)
        {
            min_lon = node.lon;
        }
        if (max_lat < node.lat)
        {
            max_lat = node.lat;
        }
        if (max_lon < node.lon)
        {
            max_lon = node.lon;
        }
    }

    double delta_lon = max_lon - min_lon;
    double delta_lat = max_lat - min_lat;
    double scale = double(CANVAS_HEIGHT) / std::min(delta_lat, delta_lon);

    std::cout << delta_lon << " " << delta_lat << std::endl;

    std::cout << min_lon << " " << min_lat << "; " << max_lon << " " << max_lat << std::endl;

    for (auto &node : nodes)
    {
        node.x = (node.lon - min_lon) * scale;
        // TODO: костыль, надо перевернуть канвас
        node.y = CANVAS_HEIGHT - (node.lat - min_lat) * scale;
    }

    for (auto &edge : edges)
    {
        auto &u = nodes[node_id_to_pos[edge.u]];
        auto &v = nodes[node_id_to_pos[edge.v]];
        edge.ux = u.x;
        edge.uy = u.y;
        edge.vx = v.x;
        edge.vy = v.y;

        edge.dist = euclidean_dist(edge.ux, edge.uy, edge.vx, edge.vy);
    }

    InitWindow(CANVAS_WIDTH, CANVAS_HEIGHT, "OMSK");
    SetTargetFPS(60);

    long start = 178263732;
    long end = 11499354530;

    auto path = dijkstra(nodes, edges, start, end);

    // Создаем переменную для камеры-
    Camera2D camera = {0};
    camera.target = {CANVAS_WIDTH / 2.0f, CANVAS_HEIGHT / 2.0f};
    camera.offset = {CANVAS_WIDTH / 2.0f, CANVAS_HEIGHT / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(camera); // Включаем режим 2D с учетом камеры

        // Рисуем узлы и ребра
        for (auto &node : nodes)
        {
            DrawCircle(node.x, node.y, 5, WHITE);
        }
        for (auto &edge : edges)
        {
            DrawLine(edge.ux, edge.uy, edge.vx, edge.vy, RED);
        }

        // Проверяем нажатие кнопок мыши для установки начальной и конечной точек
        if (IsMouseButtonPressed(0))
        {
            auto mPos = GetMousePosition();
            // Ищем ближайший узел к месту клика
            float minDistance = std::numeric_limits<float>::max();
            int closestNodeIndex = -1;

            for (int i = 0; i < nodes.size(); i++)
            {
                float dx = nodes[i].x - mPos.x;
                float dy = nodes[i].y - mPos.y;
                float distance = sqrt(dx * dx + dy * dy);

                if (distance < minDistance)
                {
                    minDistance = distance;
                    closestNodeIndex = i;
                }
            }

            if (closestNodeIndex != -1)
            {
                end = nodes[closestNodeIndex].id;
            }
            path = dijkstra(nodes, edges, start, end);
        }

        if (IsMouseButtonPressed(1))
        {
            auto mPos = GetMousePosition();
            // Ищем ближайший узел к месту клика
            float minDistance = std::numeric_limits<float>::min();
            int closestNodeIndex = -1;

            for (int i = 0; i < nodes.size(); i++)
            {
                float dx = nodes[i].x - mPos.x;
                float dy = nodes[i].y - mPos.y;
                float distance = sqrt(dx * dx + dy * dy);

                if (distance < minDistance)
                {
                    minDistance = distance;
                    closestNodeIndex = i;
                }
            }

            if (closestNodeIndex != -1)
            {
                start = nodes[closestNodeIndex].id;
            }
            path = dijkstra(nodes, edges, start, end);
        }

        // Рисуем путь
        for (size_t i = 0; i < path.size() - 1; ++i)
        {
            auto &u = nodes[node_id_to_pos[path[i]]];
            auto &v = nodes[node_id_to_pos[path[i + 1]]];
            DrawLine(u.x, u.y, v.x, v.y, GREEN);
            DrawCircle(u.x, u.y, 5, ORANGE);
        }

        // Обрабатываем управление камерой
        if (IsKeyDown(KEY_W))
            camera.offset.y += 10.0f;
        if (IsKeyDown(KEY_S))
            camera.offset.y -= 10.0f;
        if (IsKeyDown(KEY_A))
            camera.offset.x += 10.0f;
        if (IsKeyDown(KEY_D))
            camera.offset.x -= 10.0f;

        EndMode2D(); // Завершаем режим 2D

        EndDrawing();
    }

    CloseWindow();

    return 0;
}