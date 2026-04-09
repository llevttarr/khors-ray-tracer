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

void SceneParser::save_scene(const std::string& fpath) {
    std::ofstream out(fpath + ".khorssc", std::ios::binary);
    if (!out)
        throw SceneParserException("save scene exception: " + fpath);
    std::vector<SectionEntry> toc;
    toc.reserve(9);
    std::vector<char> data_buf;
    auto data_out = [&](const void* src, size_t n) {
        const char* p = reinterpret_cast<const char*>(src);
        data_buf.insert(data_buf.end(), p, p + n);
    };
    auto push_pod = [&]<typename T>(SceneSection type, const std::vector<T>& v) {
        SectionEntry e{};
        e.type = type;
        e.elem_count = v.size();
        e.byte_count = v.size() * sizeof(T);
        e.offset= sizeof(FileHeader) + 9 * sizeof(SectionEntry) + data_buf.size();
        data_out(v.data(), e.byte_count);
        toc.push_back(e);
    };
    auto push_images = [&](SceneSection type, std::vector<Image>& images) {
        SectionEntry e{};
        e.type= type;
        e.elem_count = images.size();
        e.offset = sizeof(FileHeader) + 9 * sizeof(SectionEntry) + data_buf.size();
        e.byte_count = 0;
        for (auto& img : images) {
            ImageHeader ih{
                static_cast<uint32_t>(img.w),
                static_cast<uint32_t>(img.h),
                static_cast<uint32_t>(img.channels),
                0
            };
            data_out(&ih, sizeof(ih));
            data_out(img.data.data(), img.data.size());
            e.byte_count += sizeof(ih) + img.data.size();
        }
        toc.push_back(e);
    };
    push_pod(SceneSection::TRI,render_scene.tri_v);
    push_pod(SceneSection::BVH,render_scene.bvh_v);
    push_pod(SceneSection::SPHR,render_scene.sphr_v);
    push_pod(SceneSection::MAT,render_scene.mat_v);
    push_pod(SceneSection::PRIM,render_scene.prim_v);
    push_pod(SceneSection::LIGHT,render_scene.light_v);
    push_images(SceneSection::TEX_BASE,render_scene.tex_manager.get_base());
    push_images(SceneSection::TEX_NORM,render_scene.tex_manager.get_normal());
    push_images(SceneSection::TEX_SPEC, render_scene.tex_manager.get_specular());
    FileHeader hdr{};
    hdr.magic  = MAGIC;
    hdr.version= 1;
    hdr.section_count = static_cast<uint32_t>(toc.size());
    out.write(reinterpret_cast<const char*>(&hdr),sizeof(hdr));
    out.write(reinterpret_cast<const char*>(toc.data()),sizeof(SectionEntry) * toc.size());
    out.write(data_buf.data(), data_buf.size());
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
    std::ifstream in(fpath+".khorssc", std::ios::binary);
    if (!in){
        throw SceneParserException("load scene exception:" + fpath);
    }
    RenderScene newrs{};
    auto toc = read_header(in);
    for (const auto& entry : toc) {
        in.seekg(entry.offset);
        switch (entry.type) {
            case SceneSection::TRI:read_pod_vec(in, entry, newrs.tri_v); break;
            case SceneSection::BVH:read_pod_vec(in, entry, newrs.bvh_v); break;
            case SceneSection::SPHR:read_pod_vec(in, entry, newrs.sphr_v);break;
            case SceneSection::MAT:read_pod_vec(in, entry, newrs.mat_v);break;
            case SceneSection::PRIM:read_pod_vec(in, entry, newrs.prim_v); break;
            case SceneSection::LIGHT:read_pod_vec(in, entry, newrs.light_v);break;
            case SceneSection::TEX_BASE:read_images(in, entry, newrs.tex_manager.get_base()); break;
            case SceneSection::TEX_NORM:read_images(in, entry, newrs.tex_manager.get_normal());break;
            case SceneSection::TEX_SPEC:read_images(in, entry,newrs.tex_manager.get_specular()); break;
        }
    }
    render_scene = std::move(newrs);
}

std::vector<SectionEntry> SceneParser::read_header(std::istream& in) {
    FileHeader hdr{};
    in.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (hdr.magic != MAGIC){
        throw SceneParserException("Invalid .khorssc file");
    }
    if (hdr.version != 1){
        throw SceneParserException("Unsupported .khorssc version: " + std::to_string(hdr.version));
    }
    std::vector<SectionEntry> toc(hdr.section_count);
    in.read(reinterpret_cast<char*>(toc.data()), sizeof(SectionEntry) * hdr.section_count);
    return toc;
}
void SceneParser::read_section(std::istream& in, const SectionEntry& entry,void* dest, size_t elem_size) {
    if (entry.elem_count*elem_size !=entry.byte_count){
        throw SceneParserException("read section exception - invalid byte count");
    }
    in.read(reinterpret_cast<char*>(dest), entry.byte_count);
}

void SceneParser::read_images(std::istream& in, const SectionEntry& entry,std::vector<Image>& dest) {
    dest.resize(entry.elem_count);
    for (auto& img : dest) {
        ImageHeader ih{};
        in.read(reinterpret_cast<char*>(&ih), sizeof(ih));
        img.w= static_cast<int>(ih.width);
        img.h = static_cast<int>(ih.height);
        img.channels = static_cast<int>(ih.channels);
        img.data.resize(ih.width*ih.height* ih.channels);
        in.read(reinterpret_cast<char*>(img.data.data()), img.data.size());
    }
}