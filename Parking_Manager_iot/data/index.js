// Get references to DOM elements for the hamburger menu, side menu, and reservation form
const hamburgerMenu = document.getElementById('hamburgerMenu');
const sideMenu = document.getElementById('sideMenu');
const reservationForm = document.getElementById('reservationForm');
let currentSlotId; // Variable to store the current slot ID

// Toggle the side menu and rotate the hamburger icon when clicked
hamburgerMenu.addEventListener('click', () => {
    sideMenu.classList.toggle('open');
    hamburgerMenu.classList.toggle('rotate');
});

// Load reservations from local storage or initialize with empty slots
let reservations = JSON.parse(localStorage.getItem('reservations')) || { 1: [], 2: [], 3: [] };

/**
 * Handles the process of ordering a specific parking slot.
 * @param {number} slotId - The ID of the slot being ordered.
 */
function orderSpecificSlot(slotId) {
    // Toggle the reservation form if the same slot is clicked twice
    if (currentSlotId === slotId && reservationForm.style.display === 'block') {
        reservationForm.style.display = 'none';
    } else {
        // Check if the slot is available
        if (isSlotAvailable(slotId)) {
            currentSlotId = slotId;
            clearReservationForm();
            reservationForm.style.display = 'block';
        } else {
            alert(`Parking Slot ${slotId} is already reserved.`);
        }
    }
}

/**
 * Attempts to order the first available parking slot.
 */
function orderSlot() {
    for (let slotId = 1; slotId <= 3; slotId++) {
        if (isSlotAvailable(slotId)) {
            if (currentSlotId === slotId && reservationForm.style.display === 'block') {
                reservationForm.style.display = 'none';
            } else {
                currentSlotId = slotId;
                clearReservationForm();
                reservationForm.style.display = 'block';
            }
            return;
        }
    }
    alert("No available slots.");
}

/**
 * Checks if a specific parking slot is available.
 * @param {number} slotId - The ID of the slot to check.
 * @returns {boolean} - True if the slot is available, false otherwise.
 */
function isSlotAvailable(slotId) {
    return reservations[slotId].length === 0;
}

/**
 * Submits a reservation for the current parking slot.
 */
function submitReservation() {
    const fullName = document.getElementById('fullName').value.trim();
    const carNumber = document.getElementById('carNumber').value.trim();
    const email = document.getElementById('email').value.trim();

    // Validate the input fields
    if (!validateFullName(fullName)) {
        alert("Please enter a valid full name (first and last name, letters only).");
        return;
    }

    if (!validateCarNumber(carNumber)) {
        alert("Please enter a valid car number (7-8 digits).");
        return;
    }

    if (!validateEmail(email)) {
        alert("Please enter a valid email address.");
        return;
    }

    // Generate a unique password for the reservation
    const uniquePassword = generateUniquePassword();

    const reservationData = {
        slotId: currentSlotId,
        fullName: fullName,
        carNumber: carNumber,
        email: email,
        password: uniquePassword  // Add the generated password
    };

    // Add the reservation and hide the form
    addReservation(currentSlotId, reservationData);
    reservationForm.style.display = 'none';

    // Send the reservation data to the webhook
    sendReservationToWebhook(reservationData);
}

/**
 * Generates a unique password with a specified length.
 * @param {number} [length=8] - The length of the generated password.
 * @returns {string} - The generated unique password.
 */
function generateUniquePassword(length = 8) {
    const chars = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
    let password = '';
    for (let i = 0; i < length; i++) {
        const randomIndex = Math.floor(Math.random() * chars.length);
        password += chars[randomIndex];
    }
    return password;
}

/**
 * Sends reservation data to a specified webhook.
 * @param {Object} reservationData - The data of the reservation to send.
 */
function sendReservationToWebhook(reservationData) {
    fetch('https://hook.eu2.make.com/2qd9zbx2kcgjxl00qscmc6e25rc4olfe', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(reservationData),
    })
    .then(response => response.json())
    .then(data => {
        console.log('Success:', data);
    })
    .catch((error) => {
        console.error('Error:', error);
    });
}

/**
 * Adds a reservation to a specific slot and updates the display.
 * @param {number} slotId - The ID of the slot to reserve.
 * @param {Object} userInfo - The information of the user making the reservation.
 */
function addReservation(slotId, userInfo) {
    reservations[slotId].push(userInfo);
    localStorage.setItem('reservations', JSON.stringify(reservations));

    const slotElement = document.getElementById(`slot${slotId}`);
    slotElement.classList.add('reserved');

    const reservationInfo = document.getElementById(`reservations${slotId}`);
    reservationInfo.textContent = `${userInfo.carNumber}`;

    alert(`Slot ${slotId} has been reserved.`);
}

/**
 * Clears the reservation form fields.
 */
function clearReservationForm() {
    document.getElementById('fullName').value = '';
    document.getElementById('carNumber').value = '';
    document.getElementById('email').value = '';
}

/**
 * Validates the full name field.
 * @param {string} name - The name to validate.
 * @returns {boolean} - True if the name is valid, false otherwise.
 */
function validateFullName(name) {
    const namePattern = /^[A-Za-z]+ [A-Za-z]+$/;
    return namePattern.test(name);
}

/**
 * Validates the car number field.
 * @param {string} number - The car number to validate.
 * @returns {boolean} - True if the car number is valid, false otherwise.
 */
function validateCarNumber(number) {
    const carNumberPattern = /^[0-9]{7,8}$/;
    return carNumberPattern.test(number);
}

/**
 * Validates the email field.
 * @param {string} email - The email to validate.
 * @returns {boolean} - True if the email is valid, false otherwise.
 */
function validateEmail(email) {
    const emailPattern = /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$/;
    return emailPattern.test(email);
}

/**
 * Checks if the car number and email combination already exists in any slot.
 * @param {string} carNumber - The car number to check.
 * @param {string} email - The email to check.
 * @returns {boolean} - True if a duplicate entry is found, false otherwise.
 */
function isDuplicateEntry(carNumber, email) {
    for (let slotId = 1; slotId <= 3; slotId++) {
        if (reservations[slotId].length > 0) {
            const existingReservation = reservations[slotId][0];
            if (existingReservation.carNumber === carNumber || existingReservation.email === email) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Loads the reservations from local storage and updates the display.
 */
function loadReservations() {
    for (let slotId = 1; slotId <= 3; slotId++) {
        if (reservations[slotId].length > 0) {
            const userInfo = reservations[slotId][0];
            const slotElement = document.getElementById(`slot${slotId}`);
            slotElement.classList.add('reserved');

            const reservationInfo = document.getElementById(`reservations${slotId}`);
            reservationInfo.textContent = `${userInfo.carNumber}`;
        }
    }
}

// Load the reservations initially
loadReservations();

/**
 * Periodically updates the reservation status and refreshes the display.
 */
setInterval(() => {
    reservations = JSON.parse(localStorage.getItem('reservations')) || { 1: [], 2: [], 3: [] };

    for (let slotId = 1; slotId <= 3; slotId++) {
        const slotElement = document.getElementById(`slot${slotId}`);
        const reservationInfo = document.getElementById(`reservations${slotId}`);
        
        if (reservations[slotId].length === 0 && slotElement.classList.contains('reserved')) {
            slotElement.classList.remove('reserved');
            reservationInfo.textContent = '';
        } else if (reservations[slotId].length > 0 && !slotElement.classList.contains('reserved')) {
            slotElement.classList.add('reserved');
            reservationInfo.textContent = `${reservations[slotId][0].carNumber}`;
        }
        console.log("reservations: " + JSON.stringify(reservations, null, 2));
    }
}, 5000);
