const ANGLE_MAX = 1024;
const AP = 128;
const ANGLE_INCREMENT = Math.floor(ANGLE_MAX / AP);
const ANGLE_DIR_NORMALIZATION = 24;

const canvas = document.getElementById('gameCanvas');
const ctx = canvas.getContext('2d', { willReadFrequently: true });

const WIDTH = 320;
const HEIGHT = (224 - 32)*2; // substract 32 pixels for the HUD
const HALF_HEIGHT = HEIGHT / 2;
const TEXTURE_SIZE = 64;

canvas.width = WIDTH;
canvas.height = HEIGHT;
canvas.style.width = WIDTH + 'px';
canvas.style.height = HEIGHT + 'px';

// Texture parameters
const textureState = {
    offsetX: 0,
    offsetY: 0,
    rotation: 0  // In binary angle units
};

// Flag to track movement with button clicks
var mouseClickMovement = false;

// Keyboard state tracking
const keys = {
    ArrowUp: false,
    ArrowDown: false,
    ArrowLeft: false,
    ArrowRight: false,
    Space: false
};

// Attach event listeners to the arrow buttons
const buttonArrowUp = document.getElementById("arrow_up_btn");
const buttonArrowDown = document.getElementById("arrow_down_btn");
const buttonArrowLeft = document.getElementById("arrow_left_btn");
const buttonArrowRight = document.getElementById("arrow_right_btn");
buttonArrowUp.addEventListener("click", moveUp_click);
buttonArrowDown.addEventListener("click", moveDown_click);
buttonArrowLeft.addEventListener("click", moveLeft_click);
buttonArrowRight.addEventListener("click", moveRight_click);

function resetTexturesState () {
	textureState.offsetX = 0;
	textureState.offsetY = 0;
	textureState.rotation = 0;
}

let ceilingTexture, floorTexture;

function binaryAngleToRadians (binaryAngle) {
    return (binaryAngle / ANGLE_MAX) * (2 * Math.PI);
}

function loadTexture (url) {
    return new Promise((resolve, reject) => {
        const img = new Image();
        img.onload = () => resolve(img);
        img.onerror = reject;
        img.src = url;
    });
}

async function main () {
    try {
        // Wait for both textures to load before proceeding
        [ceilingTexture, floorTexture] = await Promise.all([
            loadTexture('ceiling_texture.png'),
            loadTexture('floor_texture.png')
        ]);
        
        window.addEventListener('keydown', handleKeyDown);
        window.addEventListener('keyup', handleKeyUp);
        
        resetTexturesState();
        gameLoop();
    } catch (error) {
        console.error('Error:', error);
    }
}

function handleKeyDown (e) {
    if (e.code in keys) {
        keys[e.code] = true;
    }
}

function handleKeyUp (e) {
    if (e.code in keys) {
        keys[e.code] = false;
    }
}

function moveUp_click () {
    mouseClickMovement = true
    keys.ArrowUp = true;
}

function moveDown_click () {
    mouseClickMovement = true
    keys.ArrowDown = true;
}

function moveLeft_click () {
    mouseClickMovement = true
    keys.ArrowLeft = true;
}

function moveRight_click () {
    mouseClickMovement = true
    keys.ArrowRight = true;
}

function updateTextureState () {
    const translateSpeed = 0.1;

	if (keys.Space) {
		resetTexturesState();
	}

    // Texture translation perpendicular to rotation
    const radians = binaryAngleToRadians(textureState.rotation);
    
    if (keys.ArrowUp) {
        textureState.offsetX -= translateSpeed * Math.sin(radians);
        textureState.offsetY += translateSpeed * Math.cos(radians);
        if (mouseClickMovement == true) {
            mouseClickMovement = false;
            keys.ArrowUp = false
        }
    }
    if (keys.ArrowDown) {
        textureState.offsetX += translateSpeed * Math.sin(radians);
        textureState.offsetY -= translateSpeed * Math.cos(radians);
        if (mouseClickMovement == true) {
            mouseClickMovement = false;
            keys.ArrowDown = false
        }
    }
    
    // Texture rotation in binary angles
    if (keys.ArrowLeft) {
        textureState.rotation = (textureState.rotation + ANGLE_INCREMENT) % ANGLE_MAX;
        if (mouseClickMovement == true) {
            mouseClickMovement = false;
            keys.ArrowLeft = false
        }
    }
    if (keys.ArrowRight) {
        textureState.rotation = (textureState.rotation - ANGLE_INCREMENT) % ANGLE_MAX;
        if (mouseClickMovement == true) {
            mouseClickMovement = false;
            keys.ArrowRight = false
        }
    }
}

function createTextureImageData (texture) {
    const tempCanvas = document.createElement('canvas');
    tempCanvas.width = TEXTURE_SIZE;
    tempCanvas.height = TEXTURE_SIZE;
    const tempCtx = tempCanvas.getContext('2d');
    tempCtx.drawImage(texture, 0, 0, TEXTURE_SIZE, TEXTURE_SIZE);
    return tempCtx.getImageData(0, 0, TEXTURE_SIZE, TEXTURE_SIZE);
}

function drawTexturedPlane (y, height, texture, isCeiling) {
    const imageData = ctx.createImageData(WIDTH, height);
    const data = imageData.data;
    const radians = binaryAngleToRadians(textureState.rotation);
    const sinRotation = Math.sin(radians);
    const cosRotation = Math.cos(radians);

    const textureImageData = createTextureImageData(texture);
    const textureData = textureImageData.data;

	// true = orthogonal, false = perspective
	const useOrthogonalProjection = false;

	// Depth correction factor for Orthogonal projection
    const depthFactor = 0.8; // adjust this value to control the depth effect

	// Squish factor for Orthogonal projection
    const maxSquishFactor = 0; // Maximum squish at the end of the effect
	
	// Texture scale factor. Increase this value to make the texture looks more repetitive
	const textureScaleX = useOrthogonalProjection == true ? 3.0 : 1.0;
    const textureScaleY = useOrthogonalProjection == true ? 3.0 : 1.0;

    for (let screenY = 0; screenY < height; screenY++) {
		let rowDistance;
		let depthCorrection;
		// Orthogonal projection correction with depth
		if (useOrthogonalProjection == true) {
			const t = isCeiling ? (screenY / height) : (1 - screenY / height);
			depthCorrection = 1 / (1 + depthFactor * t);
			rowDistance = t * depthCorrection;
		}
		// Perspective correction to a vanishing point
		else {
			rowDistance = isCeiling ? (HALF_HEIGHT / (screenY + 0.1)) : (HALF_HEIGHT / (height - screenY + 0.1));
		}

		const screenYOffset = (HALF_HEIGHT - screenY) * WIDTH * 4; // Here we also invert the Y axis distribution of pixels

		// Calculate Y axis squish factor for Orthogonal projection
        let squishFactorY = 1;
		if (useOrthogonalProjection == true) {
			// Squish top to bottom
			if (isCeiling) {
				squishFactorY = 1 + (maxSquishFactor * (1 + screenY / height));
			}
			// Squish bottom to top
			else {
				squishFactorY = 1 + (maxSquishFactor * (screenY / height));
			}
		}

        for (let screenX = 0; screenX < WIDTH; screenX++) {
			let wtexX;
			// Calculate texture coordinates with Orthogonal correction and depth
			if (useOrthogonalProjection == true)
				wtexX = (((screenX - WIDTH / 2) / HEIGHT) * depthCorrection) * textureScaleX;
			// Calculate texture coordinates with Perspective correction
			else
				wtexX = ((screenX - WIDTH / 2) / HEIGHT * rowDistance) * textureScaleX;

			const wtexY = rowDistance * textureScaleY * squishFactorY;
			
            const rotatedX = wtexX * cosRotation - wtexY * sinRotation;
            const rotatedY = wtexX * sinRotation + wtexY * cosRotation;

            const textureX = Math.floor((rotatedX + textureState.offsetX) * TEXTURE_SIZE) % TEXTURE_SIZE;
            const textureY = Math.floor((rotatedY + textureState.offsetY) * TEXTURE_SIZE) % TEXTURE_SIZE;

            const textureIndex = ((textureY + TEXTURE_SIZE) % TEXTURE_SIZE * TEXTURE_SIZE + (textureX + TEXTURE_SIZE) % TEXTURE_SIZE) * 4;
            const imageDataIndex = screenYOffset + screenX * 4;

            data[imageDataIndex] = textureData[textureIndex];
            data[imageDataIndex + 1] = textureData[textureIndex + 1];
            data[imageDataIndex + 2] = textureData[textureIndex + 2];
            data[imageDataIndex + 3] = textureData[textureIndex + 3];
        }
    }

    ctx.putImageData(imageData, 0, y);
}

function drawScene () {
    // Draw ceiling
    drawTexturedPlane(0, HALF_HEIGHT, ceilingTexture, true);

    // Draw floor
    drawTexturedPlane(HALF_HEIGHT, HALF_HEIGHT, floorTexture, false);
}

function gameLoop () {
    updateTextureState();
    drawScene();
    requestAnimationFrame(gameLoop);
}

// Enable or disable image smoothing
ctx.imageSmoothingEnabled = true; // Enable anti-aliasing
ctx.imageSmoothingQuality = 'high'; // Options: 'low', 'medium', 'high'

main();