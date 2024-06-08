#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <vector>
#include <limits>
#include <queue>

struct Node
{
    long id;
    double lon;
    double lat;

    double x;
    double y;

    Node(long id, double lon, double lat) : id(id), lon(lon), lat(lat) {}
};

struct Edge
{
    long u;
    long v;

    double ux;
    double uy;

    double vx;
    double vy;

    long dist;

    Edge(long u, long v) : u(u), v(v) {}
};

std::vector<long> dijkstra(std::vector<Node> &nodes, std::vector<Edge> &edges, long start, long end);

#endif
