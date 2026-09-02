#include <tsp/tsp.h>

const char *tsp_status_string(tsp_status status)
{
    switch (status) {
    case TSP_STATUS_OK:
        return "success";
    case TSP_STATUS_INVALID_ARGUMENT:
        return "invalid argument";
    case TSP_STATUS_IO_ERROR:
        return "input/output error";
    case TSP_STATUS_INVALID_DATA:
        return "invalid graph data";
    case TSP_STATUS_LIMIT_EXCEEDED:
        return "configured limit exceeded";
    case TSP_STATUS_ARITHMETIC_OVERFLOW:
        return "arithmetic overflow";
    case TSP_STATUS_INTERNAL_ERROR:
        return "internal solver error";
    default:
        return "unknown status";
    }
}
