#pragma once
#include "do_not_edit.h"

void ClearColourBuffer(float col[4])
{
	for (int h = 0; h < PIXEL_H; h++) {
		for (int w = 0; w < PIXEL_W; w++) {
			colour_buffer[((PIXEL_H - h) * PIXEL_W * 3) + (w * 3) + 0] = col[0];
			colour_buffer[((PIXEL_H - h) * PIXEL_W * 3) + (w * 3) + 1] = col[1];
			colour_buffer[((PIXEL_H - h) * PIXEL_W * 3) + (w * 3) + 2] = col[2];
		}
	}
	cout << "colour buffer clear!" << endl;
}

void ClearDepthBuffer()
{
	for (int i = 0; i < PIXEL_H * PIXEL_W; i++) {
		depth_buffer[i] = 10.f;
	}
}

void ApplyTransformationMatrix(glm::mat4 T, vector<triangle>& tris)
{
	for (auto& tri : tris) 
	{
		tri.v1.pos = T * tri.v1.pos;
		tri.v2.pos = T * tri.v2.pos;
		tri.v3.pos = T * tri.v3.pos;
	}
}

void ApplyPerspectiveDivision(vector<triangle>& tris)
{
	/*glm::vec3 cam_pos = glm::vec3(0.f, 0.5f, 0.f);
	glm::mat4 view = glm::mat4(1.f);
	view = glm::translate(view, -cam_pos);
	for (auto& tri : tris)
	{
		tri.v1.pos = view * tri.v1.pos;
		tri.v2.pos = view * tri.v2.pos;
		tri.v3.pos = view * tri.v3.pos;
	}*/
	glm::mat4 projection = glm::mat4(1.f);
	projection = glm::perspective(glm::radians(60.f), (float)PIXEL_W / PIXEL_H, 0.1f, 10.f);
	for (auto& tri : tris)
	{
		tri.v1.pos = projection * tri.v1.pos;
		tri.v2.pos = projection * tri.v2.pos;
		tri.v3.pos = projection * tri.v3.pos;
	}

	for (auto& tri : tris)
	{
		tri.v1.pos = glm::vec4(tri.v1.pos.x / tri.v1.pos.w, tri.v1.pos.y / tri.v1.pos.w, tri.v1.pos.z / tri.v1.pos.w, tri.v1.pos.w / tri.v1.pos.w);
		tri.v2.pos = glm::vec4(tri.v2.pos.x / tri.v2.pos.w, tri.v2.pos.y / tri.v2.pos.w, tri.v2.pos.z / tri.v2.pos.w, tri.v2.pos.w / tri.v2.pos.w);
		tri.v3.pos = glm::vec4(tri.v3.pos.x / tri.v3.pos.w, tri.v3.pos.y / tri.v3.pos.w, tri.v3.pos.z / tri.v3.pos.w, tri.v3.pos.w / tri.v3.pos.w);
	}



}

void ApplyViewportTransformation(int w, int h, vector<triangle>& tris)
{
	for (auto& tri : tris)
	{
		tri.v1.pos = glm::vec4((tri.v1.pos.x + 1.f) * (w / 2) + 0.5f, (tri.v1.pos.y + 1.f) * (h / 2) + 0.5f, tri.v1.pos.z, 1);
		tri.v2.pos = glm::vec4((tri.v2.pos.x + 1.f) * (w / 2) + 0.5f, (tri.v2.pos.y + 1.f) * (h / 2) + 0.5f, tri.v2.pos.z, 1);
		tri.v3.pos = glm::vec4((tri.v3.pos.x + 1.f) * (w / 2) + 0.5f, (tri.v3.pos.y + 1.f) * (h / 2) + 0.5f, tri.v3.pos.z, 1);
	}
}

float line(glm::vec4 p1, glm::vec4 p2, glm::vec4 p3) {
	return (p2.y - p1.y) * p3.x + (p1.x - p2.x) * p3.y + p2.x * p1.y - p1.x * p2.y;
}
void ComputeBarycentricCoordinates(int px, int py, triangle t, float& alpha, float& beta, float& gamma)
{
	glm::vec4 P = glm::vec4(px, py, 1, 1);
	alpha = line(t.v2.pos, t.v3.pos, P) / line(t.v2.pos, t.v3.pos, t.v1.pos);
	beta = line(t.v1.pos, t.v3.pos, P) / line(t.v1.pos, t.v3.pos, t.v2.pos);
	gamma = line(t.v1.pos, t.v2.pos, P) / line(t.v1.pos, t.v2.pos, t.v3.pos);
}

void ShadeFragment(triangle tri, float& alpha, float& beta, float& gamma, glm::vec3& col, float& depth)
{
	col.r = tri.v1.col.r * alpha + tri.v2.col.r * beta + tri.v3.col.r * gamma;
	col.g = tri.v1.col.g * alpha + tri.v2.col.g * beta + tri.v3.col.g * gamma;
	col.b = tri.v1.col.b * alpha + tri.v2.col.b * beta + tri.v3.col.b * gamma;

	depth = tri.v1.pos.z * alpha + tri.v2.pos.z * beta + tri.v3.pos.z * gamma;

}


void Rasterise(vector<triangle> tris)
{
	for (int py = 0; py < PIXEL_H; py++)
	{
		float percf = (float)py / (float)PIXEL_H;
		int perci = percf * 100;
		std::clog << "\rScanlines done: " << perci << "%" << ' ' << std::flush;

		for (int px = 0; px < PIXEL_W; px++)
		{
			glm::vec4 P = glm::vec4(px, py, 1, 1);
			for (triangle tri : tris) {
				float a = 0;
				float b = 0;
				float y = 0;
				ComputeBarycentricCoordinates(px, py, tri, a, b, y);
				if (a >= 0 && a <= 1 && b >= 0 && b <= 1 && y >= 0 && y <= 1) {
					glm::vec3 colour = glm::vec3(0.f, 0.f, 0.f);
					float depth = 0.f;
					ShadeFragment(tri, a, b, y, colour, depth);
					if (depth < depth_buffer[px + py * PIXEL_H]) {
						writeColToDisplayBuffer(colour, px, PIXEL_H - py - 1);
						depth_buffer[px + py * PIXEL_H] = depth;
					}
				}
			}
		}
	}
	std::clog << "\rFinish rendering.           \n";
}




void render(vector<triangle>& tris)
{
	static float bgd[] = { 1.f, 1.f, 1.f, 1.f };
	ClearColourBuffer(bgd);
	ClearDepthBuffer();
	glm::vec3 oct_pos = glm::vec3(0.1f, -2.5f, -6.f);
	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, oct_pos);
	ApplyTransformationMatrix(model, tris);
	ApplyPerspectiveDivision(tris);
	ApplyViewportTransformation(PIXEL_W, PIXEL_H, tris);
	Rasterise(tris);
}

