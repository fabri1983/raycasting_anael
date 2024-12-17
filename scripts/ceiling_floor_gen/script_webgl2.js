class TextureDemo {
    constructor(canvasId) {
        this.canvas = document.getElementById(canvasId);
        this.gl = this.canvas.getContext('webgl2');

        if (!this.gl) {
            throw new Error('WebGL 2 not supported');
        }

        this.WIDTH = this.canvas.width;
        this.HEIGHT = this.canvas.height;
        this.HALF_HEIGHT = this.HEIGHT / 2;
        this.TEXTURE_SIZE = 64;

        this.ANGLE_MAX = 1024;
        this.AP = 128;
        this.ANGLE_INCREMENT = Math.floor(this.ANGLE_MAX / this.AP);

        this.textureState = {
            offsetX: 0,
            offsetY: 0,
            rotation: 0
        };

        this.keys = {
            ArrowUp: false,
            ArrowDown: false,
            ArrowLeft: false,
            ArrowRight: false,
            Space: false
        };

        this.initWebGL();
    }

    initWebGL() {
        const gl = this.gl;

        // Vertex shader with perspective projection
        const vertexShaderSource = `#version 300 es
            in vec3 a_position;
            in vec2 a_texCoord;
            
            uniform mat4 u_projectionMatrix;
            uniform mat4 u_modelViewMatrix;
            uniform vec2 u_offset;
            uniform float u_rotation;
            
            out vec2 v_texCoord;
            
            mat2 rotationMatrix(float angle) {
                return mat2(
                    cos(angle), -sin(angle),
                    sin(angle), cos(angle)
                );
            }
            
            void main() {
                // Apply rotation and translation to texture coordinates
                mat2 rotation = rotationMatrix(u_rotation);
                vec2 rotatedTexCoord = rotation * (a_texCoord - 0.5) + 0.5;
                v_texCoord = rotatedTexCoord + u_offset;
                
                // Transform vertex
                gl_Position = u_projectionMatrix * u_modelViewMatrix * vec4(a_position, 1.0);
            }
        `;

        // Fragment shader
        const fragmentShaderSource = `#version 300 es
			precision highp float;

			uniform sampler2D u_texture;

			in vec2 v_texCoord;
			out vec4 outColor;

			void main() {
				// Use fract to repeat texture coordinates
				vec2 wrappedUV = fract(v_texCoord);
				outColor = texture(u_texture, wrappedUV);
			}
        `;

        // Create shaders and program
        this.vertexShader = this.createShader(gl.VERTEX_SHADER, vertexShaderSource);
        this.fragmentShader = this.createShader(gl.FRAGMENT_SHADER, fragmentShaderSource);
        this.program = this.createProgram(this.vertexShader, this.fragmentShader);
        gl.useProgram(this.program);

        // Create plane geometry
        this.createPlaneGeometry();

        // Setup attributes and uniforms
        this.setupAttributes();
        this.setupEventListeners();
    }

    createShader(type, source) {
        const gl = this.gl;
        const shader = gl.createShader(type);
        gl.shaderSource(shader, source);
        gl.compileShader(shader);

        if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
            const error = gl.getShaderInfoLog(shader);
            gl.deleteShader(shader);
            throw new Error(`Shader compilation failed: ${error}`);
        }

        return shader;
    }

    createProgram(vertexShader, fragmentShader) {
        const gl = this.gl;
        const program = gl.createProgram();
        gl.attachShader(program, vertexShader);
        gl.attachShader(program, fragmentShader);
        gl.linkProgram(program);

        if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
            const error = gl.getProgramInfoLog(program);
            gl.deleteProgram(program);
            throw new Error(`Program linking failed: ${error}`);
        }

        return program;
    }

    createPlaneGeometry() {
        const gl = this.gl;

        // Plane vertices (centered at origin). Very large plane to simulate it is infinity
        const planeVertices = new Float32Array([
			// Positions (x, y, z)       // Tex Coords (u, v)
			-1000.0, 0.0, -1000.0,       0.0, 0.0,
			 1000.0, 0.0, -1000.0,       200.0, 0.0,
			-1000.0, 0.0,  1000.0,       0.0, 200.0,
			 1000.0, 0.0,  1000.0,       200.0, 200.0
		]);

        // Create buffers
        this.vertexBuffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, planeVertices, gl.STATIC_DRAW);

        // Create index buffer for drawing
        const indices = new Uint16Array([
            0, 1, 2,
            1, 3, 2
        ]);
        this.indexBuffer = gl.createBuffer();
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.indexBuffer);
        gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, indices, gl.STATIC_DRAW);
    }

    setupAttributes() {
        const gl = this.gl;

        // Vertex position attribute
        const positionAttributeLocation = gl.getAttribLocation(this.program, 'a_position');
        gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);
        gl.enableVertexAttribArray(positionAttributeLocation);
        gl.vertexAttribPointer(positionAttributeLocation, 3, gl.FLOAT, false, 5 * 4, 0);

        // Texture coordinate attribute
        const texCoordAttributeLocation = gl.getAttribLocation(this.program, 'a_texCoord');
        gl.enableVertexAttribArray(texCoordAttributeLocation);
        gl.vertexAttribPointer(texCoordAttributeLocation, 2, gl.FLOAT, false, 5 * 4, 3 * 4);

        // Get uniform locations
        this.uniformLocations = {
            projectionMatrix: gl.getUniformLocation(this.program, 'u_projectionMatrix'),
            modelViewMatrix: gl.getUniformLocation(this.program, 'u_modelViewMatrix'),
            texture: gl.getUniformLocation(this.program, 'u_texture'),
            offset: gl.getUniformLocation(this.program, 'u_offset'),
            rotation: gl.getUniformLocation(this.program, 'u_rotation')
        };
    }

    createProjectionMatrix() {
        // Perspective projection matrix
        const fieldOfView = Math.PI / 4; // 45 degrees
        const aspect = this.WIDTH / this.HEIGHT;
        const near = 0.1;
        const far = 100.0;

        const f = 1.0 / Math.tan(fieldOfView / 2);
        const rangeInv = 1.0 / (near - far);

        return [
            f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, (near + far) * rangeInv, -1,
            0, 0, near * far * rangeInv * 2, 0
        ];
    }

    setupEventListeners() {
        window.addEventListener('keydown', (e) => {
            if (e.code in this.keys) {
                this.keys[e.code] = true;
            }
        });

        window.addEventListener('keyup', (e) => {
            if (e.code in this.keys) {
                this.keys[e.code] = false;
            }
        });
    }

    updateTextureState() {
        const translateSpeed = 0.05;
        const rotateSpeed = (2 * Math.PI) / this.ANGLE_MAX;

        // Texture translation
        const radians = (this.textureState.rotation * rotateSpeed);
        
        if (this.keys.ArrowUp) {
            this.textureState.offsetX -= translateSpeed * Math.sin(radians);
            this.textureState.offsetY += translateSpeed * Math.cos(radians);
        }
        if (this.keys.ArrowDown) {
            this.textureState.offsetX += translateSpeed * Math.sin(radians);
            this.textureState.offsetY -= translateSpeed * Math.cos(radians);
        }
        
        // Texture rotation
        if (this.keys.ArrowLeft) {
            this.textureState.rotation = (this.textureState.rotation + this.ANGLE_INCREMENT) % this.ANGLE_MAX;
        }
        if (this.keys.ArrowRight) {
            this.textureState.rotation = (this.textureState.rotation - this.ANGLE_INCREMENT) % this.ANGLE_MAX;
        }
    }

    loadTexture(url) {
        return new Promise((resolve, reject) => {
            const img = new Image();
            img.onload = () => {
                const gl = this.gl;
                const texture = gl.createTexture();
                gl.bindTexture(gl.TEXTURE_2D, texture);

                // Set texture parameters
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

                // Upload the image to the texture
                gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, img);

                resolve(texture);
            };
            img.onerror = reject;
            img.src = url;
        });
    }

    async init() {
        try {
            // Load both textures
            this.ceilingTexture = await this.loadTexture('ceiling_texture.png');
            this.floorTexture = await this.loadTexture('floor_texture.png');

            // Start the render loop
            this.renderLoop();
        } catch (error) {
            console.error('Texture loading error:', error);
        }
    }

    renderLoop() {
        const gl = this.gl;

        // Update texture state based on key inputs
        this.updateTextureState();

        // Clear the canvas
        gl.clearColor(0, 0, 0, 1);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
        gl.enable(gl.DEPTH_TEST);

        // Calculate rotation in radians
        const rotationRadians = (this.textureState.rotation * (2 * Math.PI)) / this.ANGLE_MAX;

        // Projection matrix
        const projectionMatrix = this.createProjectionMatrix();
		gl.uniformMatrix4fv(this.uniformLocations.projectionMatrix, false, projectionMatrix);

        // Ceiling model matrix
		const modelMatrixCeiling = [
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 5, 0, 1 // Positioned 5 units above the camera
		];
        gl.uniformMatrix4fv(this.uniformLocations.modelViewMatrix, false, modelMatrixCeiling);
        
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, this.ceilingTexture);
        gl.uniform1i(this.uniformLocations.texture, 0);
        
        gl.uniform2f(this.uniformLocations.offset, 
            this.textureState.offsetX, 
            this.textureState.offsetY
        );
        gl.uniform1f(this.uniformLocations.rotation, -rotationRadians);

        // Draw ceiling plane
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.indexBuffer);
        gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT, 0);

        // Floor model matrix
		const modelMatrixFloor = [
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, -5, 0, 1 // Positioned 5 units below the camera
		];
        gl.uniformMatrix4fv(this.uniformLocations.modelViewMatrix, false, modelMatrixFloor);
        
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, this.floorTexture);
        gl.uniform1i(this.uniformLocations.texture, 0);
        
        gl.uniform2f(this.uniformLocations.offset, 
            this.textureState.offsetX, 
            this.textureState.offsetY
        );
        gl.uniform1f(this.uniformLocations.rotation, rotationRadians);

        // Draw floor plane
        gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT, 0);

        // Continue the render loop
        requestAnimationFrame(() => this.renderLoop());
    }
}

// Initialize the demo
async function main() {
    const demo = new TextureDemo('gameCanvas');
    await demo.init();
}

// Call main when the page loads
window.addEventListener('load', main);