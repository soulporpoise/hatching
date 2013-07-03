varying vec3 modelPos;
varying vec3 lightSource;
varying vec3 normal;

attribute vec3 positionIn;
attribute vec3 normalIn;

void main()
{
    vec4 posTemp = gl_ModelViewMatrix * vec4(positionIn,1.0);
	gl_Position = gl_ProjectionMatrix * posTemp;
    modelPos = posTemp.xyz;

	// send the normal to the shader
	normal = normalize(gl_NormalMatrix * normalIn);

    // push light source through modelview matrix to find its position
    // in model space and pass it to the fragment shader.
    lightSource = gl_LightSource[0].position.xyz;
}
