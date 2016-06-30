

#include "Nothing.hpp"

Nothing::Nothing(){
}

Nothing::~Nothing(){
}

void Nothing::renderLineStrip(int count){
	if(count > 1000000)
		return;
	glBindVertexArray(0);
	glDrawArrays(GL_LINE_STRIP, 0, count);
}

void Nothing::renderTriangles(int count){
	if(count > 1000000)
		return;
	glBindVertexArray(0);
	glDrawArrays(GL_TRIANGLES, 0, count);
}

void Nothing::renderPoints(int count){
	glBindVertexArray(0);
	glDrawArrays(GL_POINTS, 0, count);
}

void Nothing::renderQuad()
{
	glBindVertexArray(0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}