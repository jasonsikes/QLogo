#ifndef WORKSPACE_EXPORTS_INTERNAL_H
#define WORKSPACE_EXPORTS_INTERNAL_H


#ifdef _WIN32
#define EXPORTC extern "C" __declspec(dllexport)
#else
#define EXPORTC extern "C"
#endif

#endif