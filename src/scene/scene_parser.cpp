#include "scene_parser.h"

/*
.khorssc
tri_v
bvh_v
sphr_v
mat_v
prim_v
light_v
texture_manager

*/

void SceneParser::save_scene(const std::string& fpath){
    std::ofstream out(fpath+ ".khorssc", std::ios::binary);
    if (!out){
        throw SceneParserException("save scene exception:" + fpath);
    }

    std::vector<SectionEntry> toc(SECTION_COUNT);
    out.seekp(sizeof(FileHeader) + sizeof(SectionEntry) * SECTION_COUNT);
    int i = 0;
    toc[i].type = SceneSection::TRI;write_pod_vec(out,toc[i++], render_scene.tri_v);
    toc[i].type = SceneSection::BVH;write_pod_vec(out,toc[i++], render_scene.bvh_v);
    toc[i].type = SceneSection::SPHR;write_pod_vec(out, toc[i++], render_scene.sphr_v);
    toc[i].type = SceneSection::MAT;write_pod_vec(out, toc[i++], render_scene.mat_v);
    toc[i].type = SceneSection::PRIM;write_pod_vec(out, toc[i++], render_scene.prim_v);
    toc[i].type = SceneSection::LIGHT; write_pod_vec(out,toc[i++],render_scene.light_v);
    write_images(out,toc,render_scene.tex_manager.get_base(),SceneSection::TEX_BASE);
    write_images(out,toc,render_scene.tex_manager.get_normal(),SceneSection::TEX_NORM);
    write_images(out,toc,render_scene.tex_manager.get_specular(),SceneSection::TEX_SPEC);
    out.seekp(0);
    write_header(out, toc);
}
void SceneParser::write_header(std::ostream& out, const std::vector<SectionEntry>& toc) {
    FileHeader hdr{};
    hdr.section_count = static_cast<uint32_t>(toc.size());
    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    out.write(reinterpret_cast<const char*>(toc.data()), sizeof(SectionEntry) * toc.size());
}
void SceneParser::write_section(std::ostream& out, SectionEntry& entry, const void* data, size_t elem_size, size_t count) {
    entry.offset= out.tellp();
    entry.elem_count= count;
    entry.byte_count= elem_size * count;
    out.write(reinterpret_cast<const char*>(data),entry.byte_count);
}

void SceneParser::write_images(std::ostream& out, std::vector<SectionEntry>& toc, std::vector<Image>& images, SceneSection type) {
    SectionEntry entry{};
    entry.type = type;
    entry.offset =static_cast<uint64_t>(out.tellp());
    entry.elem_count = images.size();
    entry.byte_count = 0;
    for (auto& img : images) {
        ImageHeader ih{ 
            static_cast<uint32_t>(img.w), 
            static_cast<uint32_t>(img.h), 
            static_cast<uint32_t>(img.channels), 
            0};
        out.write(reinterpret_cast<const char*>(&ih), sizeof(ih));
        out.write(reinterpret_cast<const char*>(img.data.data()), img.data.size());
        entry.byte_count+=sizeof(ih)+img.data.size();
    }
    toc.push_back(entry);
}

void SceneParser::load_scene(const std::string& fpath){

}
