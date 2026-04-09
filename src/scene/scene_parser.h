#ifndef SCENE_PARSER_H
#define SCENE_PARSER_H

#include <string>
#include <iostream>
#include <fstream>
#include "scene.h"
static constexpr int SECTION_COUNT = 9;
static constexpr uint32_t MAGIC = 0x4B484F52;

struct SceneParserException: public std::runtime_error{
    using std::runtime_error::runtime_error;
};
enum class SceneSection {
    TRI=0,
    BVH=1,
    SPHR=2,
    MAT=3,
    PRIM=4,
    LIGHT=5,
    TEX_BASE=6,
    TEX_NORM=7,
    TEX_SPEC=8,
};
struct FileHeader {
    uint32_t magic= MAGIC;
    uint32_t version= 1;
    uint32_t section_count;
    uint32_t pad;
};
struct SectionEntry {
    SceneSection type;
    uint32_t pad;
    uint64_t offset;
    uint64_t byte_count;
    uint64_t elem_count;
};
struct ImageHeader {
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t pad;
};

class SceneParser{
public:
    SceneParser(RenderScene& rs): render_scene(rs) {}
    void save_scene(const std::string& fpath);
    void load_scene(const std::string& fpath);
private:
    RenderScene& render_scene;
    void write_header(std::ostream& out, const std::vector<SectionEntry>& toc);
    void write_section(std::ostream& out, SectionEntry& entry, const void* data, size_t elem_size, size_t count);
    void write_images(std::ostream& out, std::vector<SectionEntry>& toc, std::vector<Image>& images,SceneSection type);
    std::vector<SectionEntry> read_header(std::istream& in);
    void read_section (std::istream& in, const SectionEntry& entry, void* dest, size_t elem_size);
    void read_images (std::istream& in, const SectionEntry& entry, std::vector<Image>& dest);

   
    template<typename T>
    void write_pod_vec(std::ostream& out, SectionEntry& entry, const std::vector<T>& v) {
        write_section(out, entry, v.data(), sizeof(T), v.size());
    }
    template<typename T>
    void read_pod_vec(std::istream& in, const SectionEntry& entry, std::vector<T>& v) {
        v.resize(entry.elem_count);
        read_section(in, entry,v.data(), sizeof(T));
    }
};

#endif