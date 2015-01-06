//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef CONFIGREADER_H_
#define CONFIGREADER_H_

#include "Config.h"
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
using namespace std;

/**
 * This is a singleton class read configurations from a file.
 * The design pattern is inspired by: http://stackoverflow.com/questions/1008019/c-singleton-design-pattern
 */
class ConfigReader
{
    private:
        map<string, string> pars;

        ConfigReader()
        {
            ifstream file("./sim_config/config_file");
            string line;
            vector<string> strs;
            if (file.is_open())
            {
                getline(file, line); // Skip the title line.
                cout << "\nSimulator configuration info: " << endl;
                while (file.good())
                {
                    getline(file, line);
                    if (line.find(":") != string::npos)
                    {
                        boost::split(strs, line, boost::is_any_of(":"));
                        if (strs.size() >= 2)
                        {
                            string key = strs[0];
                            string value = strs[1];
                            cout << ">> " << key << " : " << value << endl;
                            pars.insert(pair<string, string>(key, value));
                        }
                        else
                        {
                            cout << "Configuration has errors." << endl;
                            file.close();
                            exit(EXIT_FAILURE);
                        }
                        strs.clear();
                    }
                    else {
                        // Skip the comments.
                        cout << line << endl;
                    }
                }

                file.close();
            }
            else
            {
                cout << "Configuration file ./sim_config/config_file cannot be opened." << endl;
                file.close();
                exit(EXIT_FAILURE);
            }

        }

        // Don't forget to declare these two. You want to make sure they
        // are unaccessible otherwise you may accidently get copies of
        // your singleton appearing.
        ConfigReader(ConfigReader const&); // Don't implement
        void operator=(ConfigReader const&); // Don't implement

    public:
        static ConfigReader& getInstance()
        {
            static ConfigReader instance;
            return instance;
        }

        string getVal(const string& key)
        {
            return pars.find(key)->second;
        }
};

#endif /* CONFIGREADER_H_ */
