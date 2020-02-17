#ifndef SDL_CRT_FILTER_ASSET_H
#define SDL_CRT_FILTER_ASSET_H

template <typename A>
class Asset {
public:
    Asset() {}
    virtual ~Asset() {}

    A Get() {return name;}
    void Set(A n) {name = n;}

private:
    A name;
};

template <typename A>
class Resource {
public:
    Resource() {}
    virtual ~Resource() {}

    A GetName() { return name.Get(); }
    void SetName(A n) { name.Set(n); }
    A GetUri() { return uri.Get();   }
    void SetUri(A n) { uri.Set(n);   }
    A GetType() { return type.Get(); }
    void SetType(A n) { type.Set(n); }

private:
    Asset<A> name;
    Asset<A> uri;
    Asset<A> type;
};

typedef Resource<std::string> Channel;
typedef std::map<int, Channel> ResourceMap;

#endif //SDL_CRT_FILTER_ASSET_H
