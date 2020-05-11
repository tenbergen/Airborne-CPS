#include <iostream>
#include <string.h>
#include <vector>

namespace xplane{

	class Location {
	public:
		std::string id;
		std::string ip;
		double lat;
		double lon;
		double alt;
		std::string plane;


		//ID
		void setID(std::string e) { id = e; }
		std::string getID() { return id; }

		//IP
		void setIP(std::string e) { ip = e; }
		std::string getIP() { return ip; }

		//lat
		void setLAT(double e) { lat = e; }
		double getLAT() { return lat; }

		//lon
		void setLON(double e) { lon = e; }
		double getLON() { return lon; }

		//alt
		void setALT(double e) { alt = e; }
		double getALT() { return alt; }
		
		//return plane
		std::string getPLANE() { return plane; }

		void getBytes() {
			char const *c = plane.c_str();
			size_t length = strlen(c);
			//  std::cout<<length<<std::endl;
			std::string buffer(c, length);
			//std::cout << buffer << std::endl;
		}

		int getSize() {
			char const *c = plane.c_str();
			size_t length = strlen(c);
			return length;
		}

		void BuildPlane() {
			std::string Latitude = std::to_string(lat);
			std::string Longitude = std::to_string(lon);
			std::string Altitude = std::to_string(alt);
			plane = "n" + id + "n" + ip + "n" + Latitude + "n" + Longitude + "n" + Altitude;
		}

		void deserialize(char const *c, int size) {
			std::vector<int> array;
			std::string s(c, size);
			for (int i = 0; i < s.size(); i++) {
				if (s[i] == 'n') {
					array.push_back(i);
				}
			}
			std::string id2 = s.substr(1, array.at(1) - 1);
			id = id2;
			std::string ip2 = s.substr(array.at(1) + 1, array.at(2) - (array.at(1) + 1)); //<- This is the format
			ip = ip2;
			std::string lat2 = s.substr(array.at(2) + 1, array.at(3) - (array.at(2) + 1));
			lat = std::stod(lat2);
			std::string lon2 = s.substr(array.at(3) + 1, array.at(4) - (array.at(3) + 1));
			lon = std::stod(lon2);
			std::string alt2 = s.substr(array.at(4) + 1, array.back() - (array.at(4) + 1));
			alt = std::stod(alt2);
		}




	};
	};
