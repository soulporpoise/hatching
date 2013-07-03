varying vec3 modelPos;
varying vec3 lightSource;
varying vec3 normal;

void main()
{
    vec3 C = vec3(0.0, 0.0, 0.0); // camera position

    vec3 ambientColor  = gl_LightSource[0].ambient.xyz;
    vec3 diffuseColor  = gl_LightSource[0].diffuse.xyz;
    vec3 specularColor = gl_LightSource[0].specular.xyz;

    vec3 materialColor = gl_FrontMaterial.ambient.xyz;
    vec3 materialSpec  = gl_FrontMaterial.specular.xyz;
    float shininess    = gl_FrontMaterial.shininess;

	// Phong reflection
	vec3 Lm = normalize(lightSource - modelPos);
	vec3 N = normalize(normal);
	vec3 Rm = normalize(reflect(-Lm, N));
	vec3 Vm = normalize(C - modelPos);
	
	// dot products
	float dot1 = max(0.0, dot(Lm, N));
	float dot2 = max(0.0, dot(Rm, Vm));
	
	// compute I
	vec3 ambient = materialColor * ambientColor;
	vec3 diffuse = (dot1) * materialColor * diffuseColor;
	vec3 specular = (pow(dot2, shininess)) * materialSpec * specularColor;

	vec3 result = ambient + diffuse + specular;

	// clamp result values
	//result.x = min(1.0, result.x);
	//result.y = min(1.0, result.y);
	//result.z = min(1.0, result.z);

    gl_FragColor = vec4(result, 1.0);
}
