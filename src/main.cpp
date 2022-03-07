#include "stb_image.h"
#include "vulkanctx.h"

VulkanCTX ctx;

void usage()
{
	std::cout << "Usage: vkwaifu [path to image here]\n" << std::endl;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		usage();
		return -1;
	}

	// Load image first.
	int w, h, channels;
	uint8_t *img_data = stbi_load(argv[1], &w, &h, &channels, 4);

	if (!img_data) {
		std::cout << "File does not exist! :(" << std::endl;
		return -1;
	}

	if (!ctx.Setup(w, h)) {
		std::cout << "Failed to initialize Vulkan! :(" << std::endl;
		return -1;
	}

	ctx.SetupTexture(img_data, w, h);
	stbi_image_free(img_data);
	ctx.Resize();

	VulkanUBO ubo;

	while (!ctx.ShouldClose()) {
		ctx.PollEvents();

		ubo.time += 0.002f;

		ctx.UpdateUniform(ubo);
		ctx.Update();
		ctx.DrawGraphics();
		ctx.Present();
	}

	ctx.Release();

	return 0;
}
