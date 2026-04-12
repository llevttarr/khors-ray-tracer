#include "rs_obj_parser.h"

/* - - - - */
std::unordered_map<std::string, uint32_t> rs_obj_parser::parse_mtl_names(const std::string& mtlpath, uint32_t base){
    std::unordered_map<std::string, uint32_t> map;
    std::ifstream f(mtlpath);
    std::string line;
    uint32_t idx = base;
    while (std::getline(f, line)) {
        std::istringstream ss(line);
        std::string tok; ss >> tok;
        if (tok == "newmtl") {
            std::string name; ss >> name;
            map[name] = idx++;
        }
    }
    return map;
}
void rs_obj_parser::load_obj_into_rs(const std::string& objpath,const std::string& mtlpath,RenderScene& rs){
    uint32_t mat_base = static_cast<uint32_t>(rs.mat_v.size());
    load_mtl_into_rs(mtlpath, rs);
    auto mat_map = parse_mtl_names(mtlpath, mat_base);

    std::ifstream file(objpath,std::ios::in);
    if(!file.is_open()){
        throw SceneUtilException("Could not load obj from "+objpath);
    }
    std::string line;
    std::vector<Vec3<float>> gpos;
    std::vector<Vec3<float>> gnorm;
    std::vector<Vec2<float>> guv;
    std::vector<LocalMesh> meshes;
    uint32_t cur_mat_id = mat_base;
    std::map<std::tuple<int32_t,int32_t,int32_t>, uint32_t> vert_cache;
    auto new_mesh = [&]() -> LocalMesh& {
        vert_cache.clear();
        meshes.emplace_back();
        meshes.back().mat_id = cur_mat_id;
        return meshes.back();
    };
    LocalMesh* cur = &new_mesh();

    auto get_or_add_v = [&](int32_t pi, int32_t ui, int32_t ni) -> uint32_t {
        auto key = std::make_tuple(pi, ui, ni);
        auto it  = vert_cache.find(key);
        if (it != vert_cache.end())
            return it->second;

        uint32_t idx = static_cast<uint32_t>(cur->pos.size());
        vert_cache[key] = idx;

        cur->pos.push_back(pi >= 0 ? gpos[pi] : Vec3<float>{0.f,0.f,0.f});
        cur->norm_v.push_back(ni >= 0 ? gnorm[ni]: Vec3<float>{0.f,0.f,0.f});
        cur->uv.push_back(ui >= 0 ? guv[ui]: Vec2<float>{0.f,0.f});
        cur->tang_v.push_back({0.f,0.f,0.f});
        cur->bitang_v.push_back({0.f,0.f,0.f});
        return idx;
    };

    auto parse_vert = [&](const std::string& t)-> std::tuple<int32_t,int32_t,int32_t>{
        int32_t pi = -1, ui = -1, ni = -1;
        size_t s1 = t.find('/');
        if (s1 == std::string::npos) {
            pi = obj_util::parse_face_ind(t, gpos.size());
        } else {
            pi = obj_util::parse_face_ind(t.substr(0, s1), gpos.size());
            size_t s2 = t.find('/', s1 + 1);
            if (s2 == std::string::npos) {
                std::string us = t.substr(s1 + 1);
                if (!us.empty()) ui = obj_util::parse_face_ind(us, guv.size());
            } else {
                std::string us = t.substr(s1 + 1, s2 - s1 - 1);
                std::string ns = t.substr(s2 + 1);
                if (!us.empty()) ui = obj_util::parse_face_ind(us, guv.size());
                if (!ns.empty()) ni = obj_util::parse_face_ind(ns, gnorm.size());
            }
        }
        return {pi, ui, ni};
    };
    while (std::getline(file,line)){
        if (line.empty() || line[0] == '#'){
            continue;
        }
        std::istringstream ss(line);
        std::string tok;
        ss >> tok;

        if (tok == "v") {
            Vec3<float> p; ss >> p.x >> p.y >> p.z;
            gpos.push_back(p);
        }
        else if (tok == "vn") {
            Vec3<float> n; ss >> n.x >> n.y >> n.z;
            gnorm.push_back(n);
        }
        else if (tok == "vt") {
            Vec2<float> uv; ss >> uv.x >> uv.y;
            guv.push_back(uv);
        }
        else if (tok == "usemtl") {
            std::string name; ss >> name;
            auto it = mat_map.find(name);
            uint32_t new_mat = (it != mat_map.end()) ? it->second : mat_base;
            if (new_mat != cur_mat_id) {
                cur_mat_id = new_mat;
                if (!cur->ind.empty()) cur = &new_mesh();
                else cur->mat_id = cur_mat_id;
            }
        }
        else if (tok == "f") {
            std::vector<std::string> ftoks;
            std::string ft;
            while (ss >> ft) ftoks.push_back(ft);
            if (ftoks.size() < 3){
                continue;
            }
            auto [p0,u0,n0] = parse_vert(ftoks[0]);
            uint32_t v0 = get_or_add_v(p0, u0,n0);
            for (size_t i = 1; i + 1 < ftoks.size(); ++i) {
                auto [p1,u1,n1] = parse_vert(ftoks[i]);
                auto [p2,u2,n2] = parse_vert(ftoks[i+1]);
                cur->ind.push_back(v0);
                cur->ind.push_back(get_or_add_v(p1,u1,n1));
                cur->ind.push_back(get_or_add_v(p2,u2,n2));
            }
        }
    }
    for (auto& mesh : meshes) {
        if (mesh.ind.empty()) continue;
        bool has_normals = std::any_of(mesh.norm_v.begin(), mesh.norm_v.end(),
            [](const Vec3<float>& n){ return n.x||n.y||n.z; });

        if (!has_normals) {
            for (size_t i = 0; i + 2 < mesh.ind.size(); i += 3) {
                uint32_t i0=mesh.ind[i], i1=mesh.ind[i+1], i2=mesh.ind[i+2];
                Vec3<float> fn = Vec3<float>::normalize(Vec3<float>::cross(mesh.pos[i1]-mesh.pos[i0],mesh.pos[i2]-mesh.pos[i0]));
                mesh.norm_v[i0]=mesh.norm_v[i0]+fn;
                mesh.norm_v[i1]=mesh.norm_v[i1]+fn;
                mesh.norm_v[i2]=mesh.norm_v[i2]+fn;
            }
            for (auto& n : mesh.norm_v) {
                n = Vec3<float>::normalize(n);
            }
        }
        for (size_t i = 0; i + 2 < mesh.ind.size(); i+= 3) {
            uint32_t i0=mesh.ind[i], 
            i1=mesh.ind[i+1], 
            i2=mesh.ind[i+2];
            Vec3<float> dp1=mesh.pos[i1]-mesh.pos[i0], 
            dp2=mesh.pos[i2]-mesh.pos[i0];
            Vec2<float> duv1=mesh.uv[i1]-mesh.uv[i0], 
            duv2=mesh.uv[i2]-mesh.uv[i0];
            float d = duv1.x*duv2.y - duv1.y*duv2.x;

            float r = (std::abs(d) > 1e-4f) ? 1.f/d : 0.f;
            
            
            Vec3<float> t = (dp1*duv2.y - dp2*duv1.y)*r;
            Vec3<float> b = (dp2*duv1.x - dp1*duv2.x)*r;
            
            mesh.tang_v[i0]=mesh.tang_v[i0]+t; 
            mesh.tang_v[i1]=mesh.tang_v[i1]+t; 
            mesh.tang_v[i2]=mesh.tang_v[i2]+t;

            mesh.bitang_v[i0]=mesh.bitang_v[i0]+b; 
            mesh.bitang_v[i1]=mesh.bitang_v[i1]+b; 
            mesh.bitang_v[i2]=mesh.bitang_v[i2]+b;
        }
        for (size_t i = 0; i < mesh.pos.size(); ++i) {
            Vec3<float> n = Vec3<float>::normalize(mesh.norm_v[i]);
            Vec3<float> t = Vec3<float>::normalize(mesh.tang_v[i] - n*Vec3<float>::dot(n, mesh.tang_v[i]));
            Vec3<float> b = (Vec3<float>::dot(Vec3<float>::cross(n,t), mesh.bitang_v[i]) < 0.f)? Vec3<float>::cross(n,t)*-1.f : Vec3<float>::cross(n,t);
            mesh.tang_v[i]=t; mesh.bitang_v[i]=b; mesh.norm_v[i]=n;
        }

        for (size_t i = 0; i + 2 < mesh.ind.size(); i += 3) {
            uint32_t i0=mesh.ind[i], i1=mesh.ind[i+1], i2=mesh.ind[i+2];

            Vec3<float> fn = Vec3<float>::normalize( mesh.norm_v[i0]+mesh.norm_v[i1]+mesh.norm_v[i2]);
            Vec3<float> ft = Vec3<float>::normalize(mesh.tang_v[i0]+mesh.tang_v[i1]+mesh.tang_v[i2]);
            Vec3<float> fb = Vec3<float>::normalize(
                mesh.bitang_v[i0]+mesh.bitang_v[i1]+mesh.bitang_v[i2]);

            RenderTri rt{};
            rt.v0 = {mesh.pos[i0].x, mesh.pos[i0].y, mesh.pos[i0].z, mesh.norm_v[i0].x};
            rt.v1 = {mesh.pos[i1].x, mesh.pos[i1].y, mesh.pos[i1].z, mesh.norm_v[i1].x};
            rt.v2 = {mesh.pos[i2].x, mesh.pos[i2].y, mesh.pos[i2].z, mesh.norm_v[i2].x};
            rt.t= {ft.x, ft.y, ft.z, 0.f};
            rt.b = {fb.x, fb.y, fb.z, 0.f};
            rt.n = {fn.x, fn.y, fn.z, 0.f};
            rt.uv0 = mesh.uv[i0];
            rt.uv1 = mesh.uv[i1];
            rt.uv2 = mesh.uv[i2];
            rt.matid = mesh.mat_id;
            rt.pad0  = 0;

            rs.tri_v.push_back(rt);
        }
    }

    std::vector<Prim> prims;
    prims.reserve(rs.tri_v.size() + rs.sphr_v.size());

    for (uint32_t i = 0; i < static_cast<uint32_t>(rs.tri_v.size()); ++i) {
        AABB aabb = scene_util::tri_to_aabb(rs.tri_v[i]);
        prims.push_back({aabb, scene_util::get_c_aabb(aabb),scene_util::get_type_id(0, i)});
    }
    for (uint32_t i = 0; i < static_cast<uint32_t>(rs.sphr_v.size()); ++i) {
        AABB aabb = scene_util::sphr_to_aabb(rs.sphr_v[i]);
        prims.push_back({aabb, scene_util::get_c_aabb(aabb), scene_util::get_type_id(1, i)});
    }

    rs.prim_v.clear();
    rs.bvh_v.clear();
    if (!prims.empty())
        scene_util::build_bvh(prims, 0, prims.size(), rs.prim_v, rs.bvh_v);
}
void rs_obj_parser::load_mtl_into_rs(const std::string& mtlpath,RenderScene& rs){
    std::ifstream file(mtlpath,std::ios::in);
    if(!file.is_open()){
        throw SceneUtilException("Could not load obj from "+mtlpath);
    }
    std::string dir;
    size_t sep = mtlpath.find_last_of("/\\");
    if (sep != std::string::npos){
        dir = mtlpath.substr(0,sep+1);
    }

    Mat curr{};
    curr.uv ={1.f,1.f,0.f,0.f};
    curr.tex={-1,-1,-1,-1};
    bool has_mat = false;
    std::string line;
    while (std::getline(file,line)){
        if (line.empty() || line[0] == '#'){
            continue;
        }
        std::istringstream ss(line);
        std::string tok;
        ss >> tok;
        if (tok == "newmtl") {
            if (has_mat){
                rs.mat_v.push_back(curr);
            }
            curr= Mat{};
            curr.uv = {1.f, 1.f, 0.f, 0.f};
            curr.tex = {-1, -1, -1, -1};
            has_mat = true;
        }
        else if (tok == "Ka") {
            ss >> curr.ambient.x >> curr.ambient.y >> curr.ambient.z;
            curr.ambient.w = 1.f;
        }
        else if (tok == "Kd") {
            ss >> curr.diffuse.x >> curr.diffuse.y >> curr.diffuse.z;
            curr.diffuse.w = 1.f;
        }
        else if (tok == "Ks") {
            ss >> curr.specular.x >> curr.specular.y >> curr.specular.z;
        }
        else if (tok == "Ke") {
            ss >> curr.emission.x >> curr.emission.y >> curr.emission.z;
            curr.emission.w = 1.f;
        }
        else if (tok == "Ns") {
            ss >> curr.specular.w;
        }
        else if (tok == "d") {

            // opacity not implemented yet, for now will store in diffuse[3]
            ss >> curr.diffuse.w;
        }
        else if (tok == "Tr") {
            float tr; ss >> tr;
            curr.diffuse.w = 1.f - tr;
        }
        else if (tok == "map_Kd") {
            std::string next; ss >> next;
            std::string texpath = dir + next;
            curr.tex.x = rs.tex_manager.load_base(texpath);
        }
        else if (tok == "map_Ks") {
            std::string next; ss >> next;
            std::string texpath = dir + next;
            curr.tex.y = rs.tex_manager.load_specular(texpath);
        }
        else if (tok == "map_bump" || tok == "bump") {
            std::string next; ss >> next;
            if (next == "-bm") {
                float bm;
                ss >> bm >> next;
            }
            std::string texpath = dir + next;
            curr.tex.z = rs.tex_manager.load_normal(texpath);
        }
    }
    if (has_mat){
        rs.mat_v.push_back(curr);
    }
}

/* - - - - */
