
#include <string>
#include <fstream>
#include <vector>
#include "gmic_libc.h"
#include "WM_api.h"

extern "C" {
#include "BKE_appdir.h"
}

#include "COM_GmicOperation.h"

GmicOperation::GmicOperation() : SingleThreadedOperation()
{
	this->addInputSocket(COM_DT_COLOR);
	for (int i = 0; i < 5; i++) {
		this->addInputSocket(COM_DT_VALUE);
	}

	this->addOutputSocket(COM_DT_COLOR);
	this->setResolutionInputSocketIndex(0);
	this->m_inputProgram = NULL;
	this->m_data = NULL;
}

void GmicOperation::initExecution()
{
	SingleThreadedOperation::initExecution();
	this->m_inputProgram = getInputSocketReader(0);
}

void GmicOperation::deinitExecution()
{
	this->m_inputProgram = NULL;
	SingleThreadedOperation::deinitExecution();
}

std::string readGimpCommands()
{
	std::string path = std::string(BKE_appdir_program_dir()) + "gimp.gmic";
	printf("Trying to read GIMP commands from file: %s...\n", path.c_str());

	std::ifstream filters(path);
	std::stringstream buffer;
	buffer << filters.rdbuf();
	return buffer.str();
}

const std::string& getGimpCommands()
{
	static std::string commands = readGimpCommands(); //Never freed
	return commands;
}

struct GmicFilterParam {
	std::string key;
	std::string options;
};

struct GmicFilterHelp {
	std::string name;
	std::string command;
	std::vector<GmicFilterParam*> parameters;
};

std::vector<GmicFilterHelp*> readGmicHelp()
{
	std::vector<GmicFilterHelp*> results;

	const std::string& commands = getGimpCommands();

	std::stringstream ss(commands);
	std::string to;

	const std::string tag = "#@gimp ";
	GmicFilterHelp* current = NULL;
	int mergeCount = 0;

	while (std::getline(ss, to, '\n')) {
		bool skipFirst = false;
		if (to.find(tag, 0) == 0 && to.find(" : ") != std::string::npos && current == NULL) {
			delete current;
			current = new GmicFilterHelp();
			mergeCount = 0;

			size_t start = to.find(" : ") + 3;
			size_t end = to.find(",", start);
			current->name = to.substr(tag.size(), start - tag.size() - 3);
			current->command = to.substr(start, end - start);
			results.push_back(current);
			skipFirst = true;
		}
		else if (to.find("#") != 0) {
			current = NULL;
		}

		if (current && !skipFirst) {
			size_t start = to.find(" : ");
			if (start != std::string::npos) {
				std::string body = to.substr(start + 3);
				std::string key = body.substr(0, body.find(" = "));
				std::string options = body.substr(body.find(" = ") + 3);

				if (mergeCount > 0) {
					current->parameters[current->parameters.size() - 1]->options += body;
				}
				else {
					GmicFilterParam *param = new GmicFilterParam();
					param->key = key;
					param->options = options;
					current->parameters.push_back(param);
				}

				mergeCount += std::count(body.begin(), body.end(), '{');
				mergeCount -= std::count(body.begin(), body.end(), '}');
			}
		}
	}
	printf("Found help for %i G'MIC filters\n", (int)results.size());

	return results;
}

const std::vector<GmicFilterHelp*>& getGmicHelp()
{
	static std::vector<GmicFilterHelp*> help = readGmicHelp(); //Never freed
	return help;
}

std::vector<std::string> findCommands(const char *command)
{
	const size_t len = strlen(command);
	const size_t minCommandLen = 1;

	std::vector<std::string> results;
	std::string current;

	bool inCommand = false;
	for (int i = 0; i < len; i++) {
		char tok = command[i];

		if (tok == '-') {
			inCommand = true;
		}
		else if (isspace(tok)) {
			inCommand = false;
			if (current.size() > minCommandLen) {
				results.push_back(current);
			}
			current = "";
		}

		if (inCommand && tok != '-') {
			current += tok;
		}
	}
	if (current.size() > minCommandLen) {
		results.push_back(current);
	}

	//Remove duplicates
	std::sort(results.begin(), results.end());
	results.erase(std::unique(results.begin(), results.end()), results.end());

	return results;
}

extern "C" char** explainGmicCommands(const char *command, int *resultCount)
{
	std::vector<std::string> commands = findCommands(command);
	const std::vector<GmicFilterHelp*>& help = getGmicHelp();
	std::vector<GmicFilterHelp*> found;

	for (size_t i = 0; i < commands.size(); i++) {
		for (size_t x = 0; x < help.size(); x++) {
			if (commands[i] == help[x]->command) {
				found.push_back(help[x]);
				break;
			}
		}
	}

	std::vector<std::string> results;
	for (size_t i = 0; i < found.size(); i++) {
		GmicFilterHelp *help = found[i];
		results.push_back(help->name);
		results.push_back("");

		for (size_t x = 0; x < help->parameters.size(); x++) {
			GmicFilterParam *param = help->parameters[x];
			if (param->key != "Sep") {
				results.push_back(param->key);
				results.push_back(param->options);
			}
		}
	}

	const int count = results.size();
	*resultCount = count;
	char** texts = new char*[count];

	for (size_t i = 0; i < count; i++) {
		std::string& entry = results[i];
		char *data = new char[entry.size() + 1];
		std::copy(entry.begin(), entry.end(), data);
		data[entry.size()] = '\0';
		texts[i] = data;
	}
	return texts;
}

extern "C" void freeGmicExplainCommands(char **results, int resultCount)
{
	for (int i = 0; i < resultCount; i++) {
		delete[] results[i];
	}
	delete[] results;
}

MemoryBuffer *GmicOperation::createMemoryBuffer(rcti *source)
{
	MemoryBuffer *tile = (MemoryBuffer*)this->m_inputProgram->initializeTileData(source);

	rcti rect;
	rect.xmin = 0;
	rect.ymin = 0;
	rect.xmax = source->xmax;
	rect.ymax = source->ymax;
	MemoryBuffer *result = new MemoryBuffer(COM_DT_COLOR, &rect);

	gmic_interface_image images[10]; //Hardcoded amount in C-API, why...?
	memset(&images, 0, sizeof(images));
	strcpy(images[0].name, "image0");
	images[0].width = rect.xmax;
	images[0].height = rect.ymax;
	images[0].spectrum = 4;
	images[0].depth = 1;
	images[0].is_interleaved = true;
	images[0].format = E_FORMAT_FLOAT;

	const size_t image_size = images[0].width*images[0].height*images[0].spectrum*images[0].depth * sizeof(float);
	float *data = (float*)malloc(image_size);
	memcpy(data, tile->getBuffer(), image_size);
	images[0].data = data;

	gmic_interface_options options;
	memset(&options, 0, sizeof(options));
	options.ignore_stdlib = false;
	bool abort = false;
	float progress = 0.0f;
	options.p_is_abort = &abort;
	options.p_progress = &progress;
	options.interleave_output = true;
	options.no_inplace_processing = true;
	options.output_format = E_FORMAT_FLOAT;

	const std::string& gimpCommands = getGimpCommands();
	if (gimpCommands.size() > 0) {
		printf("Found GIMP commands!\n");
		options.custom_commands = gimpCommands.c_str();
	}
	else {
		printf("No GIMP commands found\n");
	}

	std::string interpolation = "1";
	std::string command;

	// Add argument variables
	for (int i = 0; i < 5; i++) {
		float value[4];
		this->getInputSocketReader(i + 1)->readSampled(&value[0], 0, 0, COM_PS_NEAREST);
		command += "arg" + std::to_string(i + 1) + "=" + std::to_string(value[0]) + " ";
	}

	// Downscale
	int scale = std::min(std::max((int)(this->m_data->quality * 100), 10), 100);
	command += "--resize2dx " + std::to_string(scale) + "%," + interpolation + " ";
	command += "-remove[0] ";

	const bool normalize = this->m_data->flag & CMP_NODE_GMIC_NORMALIZE;
	if (normalize) {
		command += " -n 0,255 -rgb2srgb -mirror[-1] y ";
	}

	// Add the real user command
	command.append(m_data->command);

	if (normalize) {
		command += " -mirror[-1] y -srgb2rgb -n 0,1 ";
	}
	// Upscale last image to correct size and format
	command += " --resize[-1] " + std::to_string(rect.xmax) + "," + std::to_string(rect.ymax) + ",1,4," + interpolation + " ";

	unsigned int image_count = 1; 
	int error = gmic_call(command.c_str(), &image_count, &images[0], &options);
	printf("Full GMIC command: '%s', error code: %i\n", command.c_str(), error);

	for (int i = 0; i < image_count; ++i) {
		printf("Image %u: %ux%u Depth: %u Spectrum: %u\n", i, images[i].width, images[i].height, images[i].depth, images[i].spectrum);
	}

	if (error == 0) {
		// All ok, copy the final image into the result buffer
		gmic_interface_image *final_image = &images[image_count - 1];
		memcpy(result->getBuffer(), final_image->data, image_size);
	}
	else {
		// An error occurred, fill the result buffer with a solid color
		float invalid[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
		for (int y = 0; y < rect.ymax; y++) {
			for (int x = 0; x < rect.xmax; x++) {
				result->writePixel(x, y, invalid);
			}
		}
	}

	// Free all temporary buffers
	for (int i = 0; i < image_count; ++i) {
		if (images[i].data != data) {
			gmic_delete_external((float*)images[i].data);
		}
	}
	free(data);

	return result;
}

bool GmicOperation::determineDependingAreaOfInterest(rcti * /*input*/, ReadBufferOperation *readOperation, rcti *output)
{
	if (isCached()) {
		return false;
	}
	else {
		rcti newInput;
		newInput.xmin = 0;
		newInput.ymin = 0;
		newInput.xmax = this->getWidth();
		newInput.ymax = this->getHeight();
		return NodeOperation::determineDependingAreaOfInterest(&newInput, readOperation, output);
	}
}
