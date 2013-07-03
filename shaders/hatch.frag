varying float weight0;	// weight for hatch texture 0
varying float weight1;  // weight for hatch texture 1
varying float weight2;	// weight for hatch texture 2
varying float weight3;  // weight for hatch texture 3
varying float weight4;	// weight for hatch texture 4
varying float weight5;  // weight for hatch texture 5
varying vec2 texCoord1;
varying vec2 texCoord2;
varying float lightValue;

uniform sampler2D hatch012;
uniform sampler2D hatch345;

void main(void)
{
//	vec2 texCoord = gl_TexCoord[0].xy;

	float value =	texture2D(hatch012, texCoord1).r * weight0;
	value +=		texture2D(hatch012, texCoord1).g * weight1;
	value +=		texture2D(hatch012, texCoord1).b * weight2;
	value +=		texture2D(hatch345, texCoord2).r * weight3;
	value +=		texture2D(hatch345, texCoord2).g * weight4;
	value +=		texture2D(hatch345, texCoord2).b * weight5;
    
//    float value =	texture2D(hatch012, vec2(0.5,weight0)).r * weight0;
//	value +=		texture2D(hatch012, vec2(0.5,weight1)).g * weight1;
//	value +=		texture2D(hatch012, vec2(0.5,weight2)).b * weight2;
//	value +=		texture2D(hatch345, vec2(0.5,weight3)).r * weight3;
//	value +=		texture2D(hatch345, vec2(0.5,weight4)).g * weight4;
//	value +=		texture2D(hatch345, vec2(0.5,weight5)).b * weight5;
    
    //float valueTest = (weight0 + weight1 + weight2 + weight3 + weight4 + weight5)/6.0;
    //vec3 result = vec3(value,value,value)*lightValue;
    
	gl_FragColor = vec4(value,value,value,1.0); //grayscale
}