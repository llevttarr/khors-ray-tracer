#ifndef GPU_TIMER_H
#define GPU_TIMER_H
#include "comp_shader.h"

class GpuTimerScope {
public:
    explicit GpuTimerScope(GLuint query_id) {
        glBeginQuery(GL_TIME_ELAPSED, query_id);
    }

    ~GpuTimerScope() {
        glEndQuery(GL_TIME_ELAPSED);
    }

    GpuTimerScope(const GpuTimerScope&) = delete;
    GpuTimerScope& operator=(const GpuTimerScope&) = delete;
    GpuTimerScope(GpuTimerScope&&) = delete;
    GpuTimerScope& operator=(GpuTimerScope&&) = delete;
};

class GpuTimer{
private:
    GLuint query_id = 0;
public:
    GpuTimer() {
        glGenQueries(1, &query_id);
    }
    ~GpuTimer() {
        if (query_id != 0) {
            glDeleteQueries(1, &query_id);
        }
    }

    GpuTimer(const GpuTimer&) = delete;
    GpuTimer& operator=(const GpuTimer&) = delete;

    GpuTimer(GpuTimer&& other) noexcept : query_id(other.query_id) {
        other.query_id = 0;
    }

    GpuTimer& operator=(GpuTimer&& other) noexcept {
        if (this != &other) {
            if (query_id != 0) glDeleteQueries(1, &query_id);
            query_id = other.query_id;
            other.query_id = 0;
        }
        return *this;
    }

    GLuint get_id() const { return query_id; }

};

#endif // GPU_TIMER_H
