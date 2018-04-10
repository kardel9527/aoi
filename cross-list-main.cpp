#include <stdio.h>
#include <map>
#include <assert.h>

enum AoiEvent : char {
	AE_ENTER, 
	AE_LEAVE,
	AE_MOVE,
};

struct Vector2 {
	int _x;
	int _y;

	Vector2(int x, int y) : _x(x), _y(y) {}
};

struct Unit {
	int _id;
	Vector2 _pos;

	Unit(int id) : _id(id), _pos(0, 0) {}

	void on_aoi_event(int event, Unit *unit) {
		printf("unit [id:%d, x:%d, y:%d] recieved event[%d] of unit [id:%d, x:%d, y:%d]\n", 
				_id, _pos._x, _pos._y, event, unit->_id, unit->_pos._x, unit->_pos._y);
	}
};

struct Map {
	
	Map() : _start(0, 0), _width(0), _height(0), _x_view(0), _y_view(0) {}


	typedef std::map<int, Unit *> UnitMap;
	typedef std::map<int, Unit *> PositionMap;

	UnitMap _unit;
	PositionMap _x_list;
	PositionMap _y_list;

	Vector2 _start;
	int _width;
	int _height;

	int _x_view;
	int _y_view;

	void add_unit(int x, int y, Unit *unit) {
		assert(unit);
		assert(x >= _start._x && x <= (_start._x + _width));
		assert(y >= _start._y && y <= (_start._y + _height));

		// already existed
		if (_unit.find(unit->_id) != _unit.end()) return ;

		// some body already stand here.
		if (_x_list.find(x) != _x_list.end() && _y_list.find(y) != _y_list.end()) return ;

		unit->_pos._x = x;
		unit->_pos._y = y;

		UnitMap view = select_range_unit(x, y);
		for (UnitMap::iterator it = view.begin(); it != view.end(); ++it) {
			it->second->on_aoi_event(AE_ENTER, unit);
			unit->on_aoi_event(AE_ENTER, it->second);
		}


		_unit[unit->_id] = unit;
		_x_list.insert(std::pair<int, Unit *>(x, unit));
		_y_list.insert(std::pair<int, Unit *>(y, unit));
	}

	void remove_unit(Unit *unit) {
		assert(unit);

		UnitMap::iterator it = _unit.find(unit->_id);
		if (it == _unit.end()) return ;

		assert(it->second == unit);

		UnitMap view = select_range_unit(unit->_pos._x, unit->_pos._y);
		for (UnitMap::iterator itp = view.begin(); itp != view.end(); ++itp) {
			if (itp->second == unit) continue;
			itp->second->on_aoi_event(AE_LEAVE, unit);
			unit->on_aoi_event(AE_LEAVE, itp->second);
		}

		_x_list.erase(unit->_pos._x);
		_y_list.erase(unit->_pos._y);
		_unit.erase(it);
	}

	void change_location(int x, int y, Unit *unit) {
		assert(unit);
		assert(x >= _start._x && x <= (_start._x + _width));
		assert(y >= _start._y && y <= (_start._y + _height));

		// not moved.
		if (x == unit->_pos._x && y == unit->_pos._y) return ;
		
		// some body already stand hear.
		if (_x_list.find(x) != _x_list.end() && _y_list.find(y) != _y_list.end()) return ;

		UnitMap old_unit = select_range_unit(unit->_pos._x, unit->_pos._y);
		UnitMap new_unit = select_range_unit(x, y);

		for (UnitMap::iterator it = old_unit.begin(); it != old_unit.end(); ++it) {
			if (it->first == unit->_id) continue;
			if (new_unit.find(it->first) == new_unit.end()) {
				// send leave aoi
				it->second->on_aoi_event(AE_LEAVE, unit);
				unit->on_aoi_event(AE_LEAVE, it->second);
			} else {
				// send move aoi
				it->second->on_aoi_event(AE_MOVE, unit);
				new_unit.erase(it->first);
			}
		}

		for (UnitMap::iterator it = new_unit.begin(); it != new_unit.end(); ++it) {
			//just send enter aoi
			if (it->first == unit->_id) continue;
			it->second->on_aoi_event(AE_ENTER, unit);
			unit->on_aoi_event(AE_ENTER, it->second);
		}
		
		unit->_pos._x = x;
		unit->_pos._y = y;
	}

#define FILL_RANGE(C, V, VIEW, R) do { \
		PositionMap::iterator it_lower = C.lower_bound(V - VIEW);\
		PositionMap::iterator it_upper = C.lower_bound(V + VIEW);\
		if (it_upper != C.end() && it_upper->first == (V + VIEW)) \
			it_upper ++; \
		 \
		for (; it_lower != it_upper; ++it_lower) { \
			Unit *unit = it_lower->second; \
			R[unit->_id] = unit; \
		} \
	} while (0)

	UnitMap select_range_unit(int x, int y) {
		assert(x >= _start._x && x <= (_start._x + _width));
		assert(y >= _start._y && y <= (_start._y + _height));
	
		UnitMap ret;
		FILL_RANGE(_x_list, x, _x_view, ret);
		FILL_RANGE(_y_list, y, _y_view, ret);

		return ret;
	}
};

int main(int argc, char *argv[]) {
	Map map;
	map._width = 1000;
	map._height = 1000;
	map._x_view = 50;
	map._y_view = 50;

	Unit a(1), b(2);

	map.add_unit(1, 1, &a);
	map.add_unit(2, 2, &b);

	map.change_location(10, 10, &a);
	map.change_location(100, 100, &a);
	map.change_location(5, 5, &a);

	return 0;
}
