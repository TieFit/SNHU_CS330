///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures() {
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/keyboard.jpg",
		"keyboard");
	bReturn = CreateGLTexture(
		"textures/mousepad.jpg",
		"mousepad");
	bReturn = CreateGLTexture(
		"textures/desk.jpg",
		"desk");
	bReturn = CreateGLTexture(
		"textures/monitor.jpg",
		"monitor");
	bReturn = CreateGLTexture(
		"textures/wall.jpg",
		"wall");

	// bind textures to texture slots after being loaded into memory, there are 16 texture slots
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	OBJECT_MATERIAL light1;
	light1.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	light1.ambientStrength = 1.0f;
	light1.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	light1.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	light1.shininess = 1.0;
	light1.tag = "light1";

	m_objectMaterials.push_back(light1);

	OBJECT_MATERIAL light2;
	light2.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	light2.ambientStrength = 1.0f;
	light2.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	light2.specularColor = glm::vec3(0.8f, 0.8f, 0.8f);
	light2.shininess = 10.0;
	light2.tag = "light2";

	m_objectMaterials.push_back(light2);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	// m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 5.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 1.0f);

	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 5.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 1.0f);

	m_pShaderManager->setBoolValue("bUseLighting", true);

}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("desk");
	SetShaderMaterial("light1");
	m_basicMeshes->DrawPlaneMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 6.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wall");
	SetShaderMaterial("light2");
	m_basicMeshes->DrawPlaneMesh();

	// draw the mesh with transformation values

	/****************************************************************/

	/************************************** START MONITOR SECTION *****************************/

// monitor base made using cylinder
	SetTransformations(
		glm::vec3(1.0, 0.1, 1.0), // scale
		0.0, 0.0, 0.0, // rotation
		glm::vec3(0.0, 0.0, -3.0)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1); // grey

	m_basicMeshes->DrawCylinderMesh();

	// left leg for monitor base
	SetTransformations(
		glm::vec3(0.5, 0.1, 2.0), // scale
		0.0, -40.0, 0.0, // rotation
		glm::vec3(-1.3, 0.0, -1.55)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1); // grey
	m_basicMeshes->DrawPrismMesh();

	// right leg for monitor base
	SetTransformations(
		glm::vec3(0.5, 0.1, 2.0), // scale
		0.0, 40.0, 0.0, // rotation
		glm::vec3(1.3, 0.0, -1.55)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1); // grey
	m_basicMeshes->DrawPrismMesh();

	// support cylinder that connects to base
	SetTransformations(
		glm::vec3(.25, 4.0, .25), // scale
		0.0, 0.0, 0.0, // rotation
		glm::vec3(0.0, 0.0, -3.0)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1); // grey
	m_basicMeshes->DrawCylinderMesh();

	// connector piece that attatches to support cylinder and monitor screen
	SetTransformations(
		glm::vec3(0.3, 0.3, 1.0), // scale
		0.0, 0.0, 0.0, // rotation
		glm::vec3(0.0, 2.5, -2.5)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1); // grey
	m_basicMeshes->DrawBoxMesh();

	// monitor screen connected to support cylinder
	SetTransformations(
		glm::vec3(5.5, 3.5, 0.2), // scale
		0.0, 0.0, 0.0, // rotation
		glm::vec3(0.0, 3.0, -2.0)); // position
	SetShaderTexture("monitor");
	SetShaderMaterial("light2");

	m_basicMeshes->DrawBoxMesh();

	// box that covers back side of monitor so textures that wrapped are not visible
	SetTransformations(
		glm::vec3(5.5, 3.5, 0.01), // scale
		0.0, 0.0, 0.0, // rotation
		glm::vec3(0.0, 3.0, -2.11)); // position
	SetShaderColor(0, 0, 0, 1); // color is set to black

	m_basicMeshes->DrawBoxMesh();

	// box that covers upper side of monitor so textures that wrapped are not visible
	SetTransformations(
		glm::vec3(5.5, 0.21, 0.01), // scale
		90.0, 0.0, 0.0, // rotation
		glm::vec3(0.0, 4.75, -2.01)); // position
	SetShaderColor(0, 0, 0, 1); // color is set to black

	m_basicMeshes->DrawBoxMesh();

	// box that covers lower side of monitor so textures that wrapped are not visible
	SetTransformations(
		glm::vec3(5.5, 0.21, 0.01), // scale
		90.0, 0.0, 0.0, // rotation
		glm::vec3(0.0, 1.25, -2.01)); // position
	SetShaderColor(0, 0, 0, 1); // color is set to black

	m_basicMeshes->DrawBoxMesh();

	// box that covers right side of monitor so textures that wrapped are not visible
	SetTransformations(
		glm::vec3(3.5, 0.21, 0.01), // scale
		90.0, 90.0, 0.0, // rotation
		glm::vec3(2.75, 3.0, -2.01)); // position
	SetShaderColor(0, 0, 0, 1); // color is set to black

	m_basicMeshes->DrawBoxMesh();

	// box that covers left side of monitor so textures that wrapped are not visible
	SetTransformations(
		glm::vec3(3.5, 0.21, 0.01), // scale
		90.0, 90.0, 0.0, // rotation
		glm::vec3(-2.75, 3.0, -2.01)); // position
	SetShaderColor(0, 0, 0, 1); // color is set to black

	m_basicMeshes->DrawBoxMesh();

	/*********************************** END MONITOR SECTION **********************************/



	/************************************** START KEYBOARD SECTION *****************************/

	// Main part of keyboard where keys are
	SetTransformations(
		glm::vec3(4.0, 1.0, 0.1), // scale
		100.0, 0.0, 180.0, // rotation
		glm::vec3(-2.0, 0.2, 2.0)); //position
	
	SetShaderTexture("keyboard");

	m_basicMeshes->DrawBoxMesh();

	// box that covers right side of keyboard so textures that wrapped are not visible
	SetTransformations(
		glm::vec3(0.02, 1.0, 0.1), // scale
		100.0, 0.0, 0.0, // rotation
		glm::vec3(0.01, 0.2, 2.0)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1.0); // grey

	m_basicMeshes->DrawBoxMesh();

	// box that covers left side of keyboard so textures that wrapped are not visible
	SetTransformations(
		glm::vec3(0.02, 1.0, 0.1), // scale
		100.0, 0.0, 0.0, // rotation
		glm::vec3(-4.01, 0.2, 2.0)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1.0); // grey

	m_basicMeshes->DrawBoxMesh();

	// box that covers upper side of keyboard so textures that wrapped are not visible
	SetTransformations(
		glm::vec3(0.02, 4.04, 0.1), // scale
		100.0, 0.0, 90.0, // rotation
		glm::vec3(-2.0, 0.29, 1.5)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1.0); // grey

	m_basicMeshes->DrawBoxMesh();

	// box that covers lower side of keyboard so textures that wrapped are not visible
	SetTransformations(
		glm::vec3(0.02, 4.04, 0.1), // scale
		100.0, 0.0, 90.0, // rotation
		glm::vec3(-2.0, 0.11, 2.5)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1.0); // grey

	m_basicMeshes->DrawBoxMesh();

	// box that covers bottom side of keyboard so textures that wrapped are not visible
	SetTransformations(
		glm::vec3(1.035, 4.04, 0.01), // scale
		100.0, 0.0, 90.0, // rotation
		glm::vec3(-2.0, 0.15, 1.99)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1.0); // grey

	m_basicMeshes->DrawBoxMesh();

	// wrist rest for keyboard
	SetTransformations(
		glm::vec3(4.0, 0.4, 0.05), // scale
		100.0, 0.0, 0.0, // rotation
		glm::vec3(-2.0, 0.1, 2.69)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1.0); // grey

	m_basicMeshes->DrawBoxMesh();

	// right support leg for keyboard
	SetTransformations(
		glm::vec3(0.2, 0.03, 0.1), // scale
		100.0, 90.0, 0.0, // rotation
		glm::vec3(-0.1, 0.15, 1.5)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1.0); // grey

	m_basicMeshes->DrawBoxMesh();

	// left support leg for keyboard
	SetTransformations(
		glm::vec3(0.2, 0.03, 0.1), // scale
		100.0, 90.0, 0.0, // rotation
		glm::vec3(-3.9, 0.15, 1.5)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1.0); // grey

	m_basicMeshes->DrawBoxMesh();

	/************************************** END KEYBOARD SECTION *****************************/


	/************************************** START MOUSE SECTION *****************************/

	// mouse body made with flattened sphere
	SetTransformations(
		glm::vec3(0.6, 0.15, 0.9), // scale 
		0.0, 0.0, 0.0, // rotation
		glm::vec3(2.3, 0.18, 2.0)); // position
	SetShaderColor(0.2, 0.2, 0.2, 1.0); // dark gray

	m_basicMeshes->DrawSphereMesh();

	// mouse scroll wheel made with cylinder
	SetTransformations(
		glm::vec3(0.1, 0.1, 0.1), // scale 
		90.0, 0.0, 90.0, // rotation 
		glm::vec3(2.35, 0.25, 1.5)); // position
	SetShaderColor(0.0, 0.0, 0.0, 1.0); // black

	m_basicMeshes->DrawCylinderMesh();

	// mouse side button closest to scroll wheel
	SetTransformations(
		glm::vec3(0.2, 0.02, 0.02), // scale
		90.0, 45.0, 90.0, // rotation
		glm::vec3(1.82, 0.26, 1.9)); // position
	SetShaderColor(0, 0, 0, 1); // color is black

	m_basicMeshes->DrawBoxMesh();

	// mouse side button furthest from scroll wheel
	SetTransformations(
		glm::vec3(0.2, 0.02, 0.02), // scale
		90.0, 45.0, 90.0, // rotation
		glm::vec3(1.82, 0.26, 2.15)); // position
	SetShaderColor(0, 0, 0, 1); // color is black

	m_basicMeshes->DrawBoxMesh();


	/************************************** END MOUSE SECTION *****************************/


	/************************************** START MOUSEPAD SECTION *****************************/

	SetTransformations(
		glm::vec3(10.0, 0.05, 5.0), // scale
		0.0, 0.0, 0.0, // rotation
		glm::vec3(0.0, 0.0, 2.0)); // position
	SetShaderColor(0, 0, 0, 1); // color is set to black

	m_basicMeshes->DrawBoxMesh();

	/************************************** END MOUSEPAD SECTION *****************************/

};