#include <assert.h>
#include <iostream>
#include <algorithm>
#include <windows.h>
#include <stdio.h>
#include <map>
#include <vector>
#include <time.h>

#pragma comment(lib, "winmm.lib ")
#pragma comment(lib,"Ws2_32.lib")

typedef unsigned char uint8;
typedef char int8;
typedef unsigned short uint16;
typedef short int16;
typedef unsigned short uint16;
typedef unsigned uint32;
typedef int int32;
typedef unsigned long long uint64;
typedef long long int64;

struct Vector2 {
	int32 _x;
	int32 _y;

	Vector2() : _x(0), _y(0) {}
	Vector2(int32 x, int32 y) : _x(x), _y(y) {}
};

inline uint64 make_map_id(uint8 map_type, uint32 map_id, uint32 area_id) {

	return ((uint64)map_type) << 56 | ((uint64)map_id) <<  24 | ((uint64)area_id) & 0x00FFFFFF; 
}

inline uint8 get_map_type(uint64 map_id) {
	return (uint8)((map_id >> 56) & 0xFF);
}

inline uint32 get_map_real_id(uint64 map_id) {
	return (uint32)((map_id >> 24) & 0xFFFFFFFF);
}

inline uint32 get_map_area_id(uint64 map_id) {
	return (uint32)(map_id & 0xFFFFFF);
}

class MapUnit {
public:
	enum MapUnitType : uint8 {
		MUT_ITEM = 0,
		MUT_MONSTER = 1,
		MUT_PLAYER = 2,
		MUT_BUIDING = 3,
		MUT_MAX = 4,
	};

public:
	MapUnit() : _guid(0), _type(MUT_MAX), _map_id(0) {}
	MapUnit(int64 guid) : _guid(guid), _type(MUT_MAX), _map_id(0) {}
	MapUnit(int64 guid, int32 type) : _guid(guid), _type(type), _map_id(0) {}

	void type(int32 type) { _type = type; }
	int32 type() const { return _type; }

	void guid(int64 guid) { _guid = guid; }
	int64 guid() const { return _guid; }

	void pos(int x, int y) { _pos._x = x; _pos._y = y; }
	void pos(const Vector2 &pos) { _pos = pos; }
	const Vector2& pos() const { return _pos; }

	void map_id(uint64 id) { _map_id = id; }
	uint64 map_id() const { return _map_id; }

private:
	int64 _guid;
	int32 _type;
	Vector2 _pos;
	uint64 _map_id;
};

// item that can drop on the map.
class Item : public MapUnit {
public:
	Item() : MapUnit(0, MUT_ITEM) {}
};

class Monster : public MapUnit {
public:
	Monster() : MapUnit(0, MUT_MONSTER) {}
};

class Player : public MapUnit {
public:
	Player() : MapUnit(0, MUT_PLAYER) {}
};

struct MapProto {

};

class Map {
	struct MapArea {
		typedef std::map<int64, MapUnit *> MapUnitMap;
		MapUnitMap _unit_map[MapUnit::MUT_MAX];

		bool add_unit(MapUnit *unit) {
			assert(unit->type() >= 0 && unit->type() < MapUnit::MUT_MAX);
			MapUnitMap &data_map = _unit_map[unit->type()];
			return data_map.insert(std::make_pair<int64, MapUnit *>(unit->guid(), unit)).second;
		}

		bool remove_unit(MapUnit *unit) {
			assert(unit->type() >= 0 && unit->type() < MapUnit::MUT_MAX);
			MapUnitMap &data_map = _unit_map[unit->type()];
			MapUnitMap::iterator it = data_map.find(unit->guid());
			if (it == data_map.end()) return false;
			data_map.erase(it);

			return true;
		}
	};

public:
	Map() : _type(0), _seed(0), _area_cnt(0), _area(0) {}
	~Map() { _area_cnt = 0; if (_area) { delete[] _area; _area = 0;} }

	void init(const MapProto &proto) {
		_type = 1;
		_origin_point._x = 0;
		_origin_point._y = 0;

		_hw._x = 79;
		_hw._y = 29;

		_area_hw._x = 20;
		_area_hw._y = 10;

		_area_hw_cnt._x = (_hw._x + _area_hw._x) / _area_hw._x;
		_area_hw_cnt._y = (_hw._y + _area_hw._y) / _area_hw._y;

		_area_cnt = _area_hw_cnt._x * _area_hw_cnt._y;
		_area = new MapArea[_area_cnt];
	}

	void add_unit(int32 x, int32 y, MapUnit *unit) {
		assert(unit);
		assert(x >= _origin_point._x && x <= (_origin_point._x + _hw._x));
		assert(y >= _origin_point._y && y <= (_origin_point._y + _hw._y));
		assert(unit->map_id() == 0);
		x = x - _origin_point._x;
		y = y - _origin_point._y;

		uint32 area_id = make_area_id(x, y);
		assert(area_id >=0 && area_id < _area_cnt);

		MapArea &area = _area[area_id];
		if (!area.add_unit(unit)) return ;

		unit->map_id(make_map_id(_type, ++_seed, area_id));
		unit->pos(x, y);

		// notice aoi
		std::vector<MapUnit *> player = get_neighbor_area_unit(area_id, MapUnit::MUT_PLAYER);
		for (std::vector<MapUnit *>::iterator it = player.begin(); it != player.end(); ++it) {
			MapUnit *plr = *it;
			if (plr->guid() != unit->guid()) printf("enter aoi[%d] [%llu->%llu]\n", area_id, unit->guid(), plr->guid());
		}
	}

	void send_msg_to_around(uint32 area_id, const char *data, uint32 len, MapUnit *except = 0) {
		std::vector<MapUnit *> player = get_neighbor_area_unit(area_id, MapUnit::MUT_PLAYER);
		for (std::vector<MapUnit *>::iterator it = player.begin(); it != player.end(); ++it) {
			MapUnit *plr = *it;
			if (except && plr->guid() == except->guid()) continue;

			// send msg.
		}
	}

	void unit_change_location(int32 x, int32 y, MapUnit *unit) {
		assert(unit);
		assert(x >= _origin_point._x && x <= (_origin_point._x + _hw._x));
		assert(y >= _origin_point._y && y <= (_origin_point._y + _hw._y));
		assert(unit->map_id() != 0);

		if (x == unit->pos()._x && y == unit->pos()._y) return ;

		unit->pos(x, y);

#define SND_AREA_MSG(id, fmt, unit_guid) do { \
			MapArea &area = _area[id]; \
			for (MapArea::MapUnitMap::iterator it = area._unit_map[MapUnit::MUT_PLAYER].begin(); it != area._unit_map[MapUnit::MUT_PLAYER].end(); ++it) {\
				if (unit_guid != it->second->guid()) printf(fmt, (id), unit_guid, it->second->guid()); \
			} \
		} while (0)

		uint32 new_area_id = make_area_id(x, y);
		assert(new_area_id >= 0 && new_area_id < _area_cnt);
		uint32 old_area_id = get_map_area_id(unit->map_id());
		if (new_area_id == old_area_id) {
			std::vector<uint32> neighbor = get_neighbor_area(old_area_id);
			for (std::vector<uint32>::iterator it = neighbor.begin(); it != neighbor.end(); ++it) {
				uint32 area_id = *it;
				MapArea *tmp = &_area[area_id];
				SND_AREA_MSG(area_id, "move aoi[%d] [%llu->%llu]\n", unit->guid());
			}
			return ;
		}

		if (!_area[old_area_id].remove_unit(unit)) return ;
		if (!_area[new_area_id].add_unit(unit)) return ;

		unit->map_id(make_map_id(_type, get_map_real_id(unit->map_id()), new_area_id));

		std::vector<uint32> old_neighbor = get_neighbor_area(old_area_id);
		std::vector<uint32> new_neighbor = get_neighbor_area(new_area_id);

		std::map<uint32, bool> old_neighbor_map;
		for (std::vector<uint32>::iterator it = old_neighbor.begin(); it != old_neighbor.end(); ++it) {
			old_neighbor_map[*it] = true;	
		}

		std::map<uint32, bool> new_neighbor_map;
		for (std::vector<uint32>::iterator it = new_neighbor.begin(); it != new_neighbor.end(); ++it) {
			new_neighbor_map[*it] = true;	
		}

		for (std::map<uint32, bool>::iterator it = old_neighbor_map.begin(); it != old_neighbor_map.end(); ++it) {
			if (new_neighbor_map.find(it->first) == new_neighbor_map.end()) {
				// send leave aoi message.
				SND_AREA_MSG(it->first, "leave aoi[%d] [%llu->%llu]\n", unit->guid());
			} else {
				// send move aoi message.
				SND_AREA_MSG(it->first, "move aoi[%d] [%llu->%llu]\n", unit->guid());
			}
		}

		for (std::map<uint32, bool>::iterator it = new_neighbor_map.begin(); it != new_neighbor_map.end(); ++it) {
			if (old_neighbor_map.find(it->first) == old_neighbor_map.end()) {
				// send enter aoi message.leave
				SND_AREA_MSG(it->first, "enter aoi[%d] [%llu->%llu]\n", unit->guid());
			}
		}
	}

	void remove_unit(MapUnit *unit) {
		assert(unit);
		uint32 area_id = get_map_area_id(unit->map_id());
		assert(area_id >= 0 && area_id < _area_cnt);
		MapArea &area = _area[area_id];
		if (!area.remove_unit(unit)) return ;

		// notice aoi
		std::vector<MapUnit *> player = get_neighbor_area_unit(area_id, MapUnit::MUT_PLAYER);
		for (std::vector<MapUnit *>::iterator it = player.begin(); it != player.end(); ++it) {
			MapUnit *plr = *it;
			printf("leave aoi[%d] [%llu->%llu]\n", area_id, unit->guid(), plr->guid());
		}
	}

	std::vector<MapUnit *> get_neighbor_area_unit(uint32 area_id, int32 unit_type) {
		assert(unit_type >= 0 && unit_type < MapUnit::MUT_MAX);
		std::vector<uint32> neighbor_area_id = get_neighbor_area(area_id);
		std::vector<MapUnit *> ret;
		for (std::vector<uint32>::iterator it = neighbor_area_id.begin(); it != neighbor_area_id.end(); ++it)  {
			assert(*it >= 0 && *it < _area_cnt);

			MapArea::MapUnitMap &data = _area[*it]._unit_map[unit_type];
			for (MapArea::MapUnitMap::iterator iter = data.begin(); iter != data.end(); ++iter) {
				ret.push_back(iter->second);
			}

		}

		return ret;
	}

private:
	uint32 make_area_id(int32 x, int32 y) {
		return (y / _area_hw._y) *_area_hw_cnt._x + x / _area_hw._x;
	}

	std::vector<uint32> get_neighbor_area(uint32 area_id) {
		std::vector<uint32> ret;
		ret.push_back(area_id);
		bool has_left = false, has_right = false;
		if ((area_id % _area_hw_cnt._x)  != 0) {
			has_left = true;
			ret.push_back(area_id - 1);
		}

		if ((area_id % _area_hw_cnt._x) != (_area_hw_cnt._x - 1)) {
			has_right = true;
			ret.push_back(area_id + 1);
		}
		
		if ((area_id / _area_hw_cnt._x) != 0) {
			ret.push_back(area_id - _area_hw_cnt._x);
			if (has_left) {
				ret.push_back(area_id - _area_hw_cnt._x - 1);
			}

			if (has_right) {
				ret.push_back(area_id - _area_hw_cnt._x + 1);
			}
		}

		if ((area_id / _area_hw_cnt._x) != (_area_hw_cnt._y - 1)) {
			ret.push_back(area_id + _area_hw_cnt._x);
			if (has_left) {
				ret.push_back(area_id + _area_hw_cnt._x - 1);
			}

			if (has_right) {
				ret.push_back(area_id + _area_hw_cnt._x + 1);
			}
		}

		return ret;
	}

private:
	uint8 _type;
	uint32 _seed;

	Vector2 _origin_point;
	Vector2 _hw; // height and width

	Vector2 _area_hw;
	Vector2 _area_hw_cnt;
	uint32 _area_cnt;
	MapArea *_area;
};

class MapMgr {

};

void aoi_test() {
	MapProto proto;
	Map map;
	map.init(proto);

	MapUnit *unit1 = new MapUnit(1, MapUnit::MUT_PLAYER);
	MapUnit *unit2 = new MapUnit(2, MapUnit::MUT_PLAYER);
	MapUnit *unit3 = new MapUnit(3, MapUnit::MUT_PLAYER);

	map.add_unit(20, 20, unit1);
	map.add_unit(20, 10, unit2);
	map.add_unit(20, 15, unit3);

	map.unit_change_location(20, 11, unit2);
	map.unit_change_location(20, 9, unit2);
	map.unit_change_location(20, 10, unit2);
	map.unit_change_location(60, 25, unit2);
	map.remove_unit(unit2);
}

/*
template<typename T>
class PriorityQuueu {
public:
	class Node {
	public:
		Node() : _idx(0) {}
		virtual ~Node() {}

		void idx(uint32 idx) { _idx = idx; }
		uint32 idx() const { return _idx; }

	private:
		uint32 _idx;
	};

public:
	T pop() {
		T ret = _data.front();
		_data[0] = _data.back();
		_data.pop_back();
		if (_data.size() > 0) filter_down(0);
		return ret;
	}

	void push(const T &t) {
		_data.push_back(t);
		filter_up(_data.size() - 1);
	}

	size_t size() const { return _data.size(); }

	bool empty() const { return _data.empty(); }

private:
	void filter_up(uint32 idx) {
		T node = _data[idx];
		while (idx > 0) {
			uint32 parent = (idx - 1) >> 1;
			assert(parent >= 0 );
			if (_data[parent] >= node) break;

			_data[idx] = _data[parent];
			idx = parent;
		}

		assert(idx >= 0);
		_data[idx] = node;
	}

	void filter_down(uint32 idx) {
		T node = _data[idx];
		while (true) {
			uint32 lchild = (idx << 1) + 1;
			uint32 rchild = lchild + 1;
			uint32 child = 0;

			if (rchild < _data.size()) {
				child = ((_data[lchild] < _data[rchild]) ? rchild : lchild);
			} else if (lchild < _data.size()) {
				child = lchild;
			} else {
				break;
			}

			if (node >= _data[child]) break;

			_data[idx] = _data[child];
			idx = child;
		}

		_data[idx] = node;
	}

private:
	std::vector<T> _data;
};

void test_heap_sort() {
	PriorityQuueu<int> queue;

	for (int i = 0; i < 100; ++i) {
		queue.push(i + 1);
	}

	while (!queue.empty())
		printf("%d\n", queue.pop());
}
*/

int main(int argc, char *argv[]) {

	aoi_test();
	//test_heap_sort();

	return 0;
}
