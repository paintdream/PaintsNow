// ISceneExplorer.h
// By PaintDream
// 2018-2-24
//

#ifndef __ISCENEEXPLORER_H__
#define __ISCENEEXPLORER_H__

#include <string>

class ISceneExplorer {
public:
	virtual ~ISceneExplorer();
	enum LOG_LEVEL { LOG_TEXT, LOG_WARNING, LOG_ERROR };
	virtual void WriteLog(LOG_LEVEL logLevel, const std::string& log) = 0;
};


#endif // __ISCENEEXPLORER_H__