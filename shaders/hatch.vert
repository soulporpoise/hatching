varying float weight0;	// weight for hatch texture 0
varying float weight1;  // weight for hatch texture 1
varying float weight2;	// weight for hatch texture 2
varying float weight3;  // weight for hatch texture 3
varying float weight4;	// weight for hatch texture 4
varying float weight5;  // weight for hatch texture 5
varying vec2 texCoord1;
varying vec2 texCoord2;
varying float lightValue;

attribute vec3 positionIn;
attribute vec3 normalIn;
attribute vec2 texcoordIn;

void main()
{
	vec4 posTemp = gl_ModelViewMatrix * vec4(positionIn, 1);
	gl_Position = gl_ProjectionMatrix * posTemp;

//    texCoord1 = vec2(gl_TexCoord[0]);
//    texCoord2 = vec2(gl_TexCoord[1]);

    texCoord1 = texcoordIn;
    texCoord2 = texcoordIn;
	
    vec3 ambientColor  = gl_LightSource[0].ambient.xyz;
    vec3 diffuseColor  = gl_LightSource[0].diffuse.xyz;
    vec3 specularColor = gl_LightSource[0].specular.xyz;
    
    vec3 materialColor = gl_FrontMaterial.ambient.xyz;
    vec3 materialSpec  = gl_FrontMaterial.specular.xyz;
    float shininess    = gl_FrontMaterial.shininess;
    
    vec4 vertex = gl_ModelViewMatrix * vec4(positionIn,1.0);
	vec3 normal = normalize(gl_NormalMatrix * normalIn);
	vec3 light = normalize(gl_LightSource[0].position.xyz - vertex.xyz);
	vec3 refl = normalize(reflect(-light,normal));
    vec3 view = normalize(-vertex.xyz);
    
	lightValue = max(dot(normal, light), 0.0);
	float hatchLevel = lightValue * 6.0;

//	float dot1 = max(0.0, dot(light, normal));
//    float dot2 = max(0.0, dot(refl, view));
//    
//    vec3 ambient = materialColor * ambientColor;
//    vec3 diffuse = (dot1) * materialColor * diffuseColor;
//    vec3 specular = (pow(dot2, shininess)) * materialSpec * specularColor;
//    
//    vec3 result = diffuse;
//    float hatchLevel = (result.r + result.g + result.b)/3.0 * 6.0;
    
    // lightest tone (implicit white)
	if (hatchLevel >= 5.0)
	{
		weight0 = 1.0;
        weight1 = 0.0;
        weight2 = 0.0;
        weight3 = 0.0;
        weight4 = 0.0;
        weight5 = 0.0;
	}
    // lighter tone
	else if (hatchLevel >= 4.0)
	{
		weight0 = 1.0 - (5.0 - hatchLevel);
		weight1 = 1.0 - weight0;
        weight2 = 0.0;
        weight3 = 0.0;
        weight4 = 0.0;
        weight5 = 0.0;
	}
    // light tone
	else if (hatchLevel >= 3.0)
	{
        weight0 = 0.0;
		weight1 = 1.0 - (4.0 - hatchLevel);
		weight2 = 1.0 - weight1;
        weight3 = 0.0;
        weight4 = 0.0;
        weight5 = 0.0;
	}
    // dark tone
	else if (hatchLevel >= 2.0)
	{
        weight0 = 0.0;
        weight1 = 0.0;
		weight2 = 1.0 - (3.0 - hatchLevel);
		weight3 = 1.0 - weight2;
        weight4 = 0.0;
        weight5 = 0.0;
	}
    // darker tone
	else if (hatchLevel >= 1.0)
	{
        weight0 = 0.0;
        weight1 = 0.0;
        weight2 = 0.0;
		weight3 = 1.0 - (2.0 - hatchLevel);
		weight4 = 1.0 - weight3;
        weight5 = 0.0;
	}
    // darkest tone
	else
	{
        weight0 = 0.0;
        weight1 = 0.0;
        weight2 = 0.0;
        weight3 = 0.0;
		weight4 = 1.0 - (1.0 - hatchLevel);
		weight5 = 1.0 - weight4;
	}	
}