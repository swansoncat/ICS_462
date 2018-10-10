

#include "ARegion.h"

ARegion::ARegion(float x, float y, int i, bool b)
{
	x_coord = x;
	y_coord = y;
	id = i;
	passable = b;
	
}

ARegionMap::ARegionMap()
{
	double squareRoot = sqrt(BZDBCache::worldSize);
	int half = squareRoot / 2;
	leftMargin = 0 - half;
	rightMargin = 0 + half;
	topMargin = 0 - half;
	bottomMargin = 0 + half;
	horizontalTiles = ceil(squareRoot / BZDBCache::tankRadius);
	verticalTiles = ceil(squareRoot / BZDBCache::tankRadius);
	totalTiles = horizontalTiles * verticalTiles;

	for (int i = 0 ; i < verticalTiles ; i++)
	{
		int count = 1;
		for (int j = 0 ; j < horizontalTiles ; j++)
		{
			float tankHalf = BZDBCache::tankRadius / 2;
			float xCenter = leftMargin + tankHalf + (j * BZDBCache::tankRadius);
			float yCenter = topMargin + tankHalf + (i * BZDBCache::tankRadius);
			const float* coord;
			coord[0] = xCenter;
			coord[1] = yCenter;


			if (World::getWorld()->inBuilding(coord, tankHalf, 0) != NULL)
			{
				ARegion a = new ARegion(xCenter, yCenter, count, false);	
			}
			else
			{
				ARegion a = new ARegion(xCenter, yCenter, count, true);
			}

		}
		count++;
	}		



}

float* ARegionMap::setTarget(float* target)
{
	float xTarg = target[0];
	float yTarg = target[1];
	int targetTileX = (xTarg - leftMargin) / BZDBCache::tankRadius;
	int targetTileY = (yTarg - topMargin) / BZDBCache::tankRadius;
}






