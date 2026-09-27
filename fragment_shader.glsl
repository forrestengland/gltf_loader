precision mediump float;

varying vec3 vertexNormal;

void main()
{
    vec3 lightDirection =
        normalize(vec3(1.0, 1.0, 1.0));

    float brightness =
        max(
            dot(normalize(vertexNormal), lightDirection),
            0.0
        );

    float ambient = 0.2;

    vec3 baseColor = vec3(0.1, 0.8, 0.4);

    gl_FragColor = vec4(
        baseColor * (ambient + brightness),
        1.0
    );
}

/* precision mediump float;

void main()
{
    gl_FragColor = vec4(0.2, 0.8, 0.3, 1.0);
} */
