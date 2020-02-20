#ifndef SDL_CRT_FILTER_RESOURCEROLLER_HPP
#define SDL_CRT_FILTER_RESOURCEROLLER_HPP
#include <Asset.h>

class ResourceRoller {
public:
    ResourceRoller() { chn = 0; }
    void        Up() { ++chn; }
    void        Dw() { --chn; }
    nint        size() { return resources.size(); }
    nint        Pos() { current(); return !resources.empty() ? chn: 0; }
    void        Go(nint p) { chn = p; }
    void Add(std::string name, std::string uri, std::string type) { Append(resources, name, uri, type); }
    void Del(nint p) { Go(p); current(); resources.erase(Pos()); }
    ResourceMap Get() { return resources; }

    Channel     current() {
        auto itr = resources.find(chn);
        while (itr == resources.end() && !resources.empty()) {
            if(chn > itr->first || chn < resources.begin()->first) Go(resources.begin()->first); else Up();
            itr = resources.find(chn);
        }
        return !resources.empty() ? resources.at(chn): empty;
    }

private:
    nint chn;
    Channel empty;
    ResourceMap resources;
    static void Append(ResourceMap& res, std::string name, std::string uri, std::string type) {
        Channel ch;
        ch.SetName(name);
        ch.SetUri (uri);
        ch.SetType(type);
        res[res.size()] = ch;
    }
};

#endif //SDL_CRT_FILTER_RESOURCEROLLER_HPP
