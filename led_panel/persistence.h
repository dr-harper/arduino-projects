#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "config.h"

void initDefaultConfig(GridConfig &cfg);
void loadGridConfig(GridConfig &cfg);
void saveGridConfig(const GridConfig &cfg);

#endif // PERSISTENCE_H
