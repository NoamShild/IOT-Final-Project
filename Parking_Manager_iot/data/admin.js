// Load reservations from local storage or initialize with empty slots
let reservations = JSON.parse(localStorage.getItem('reservations')) || { 1: [], 2: [], 3: [] };

/**
 * Frees a parking slot and updates the reservations.
 * @param {number} slotId - The ID of the slot to be freed.
 */
function freeSlot(slotId) {
    // Check if the slot is already empty
    if (reservations[slotId].length === 0) {
        alert(`Slot ${slotId} is already empty.`);
        return;
    }

    // Confirm with the user if they want to free the slot
    if (confirm(`Are you sure you want to free Slot ${slotId}?`)) {
        // Clear the reservation for the specified slot
        reservations[slotId] = [];
        localStorage.setItem('reservations', JSON.stringify(reservations));

        // Update the admin information display
        loadAdminInfo();

        alert(`Slot ${slotId} has been freed.`);
    }
}

/**
 * Fetches the current sensor status from the server and updates the status display.
 */
function updateSensorStatus() {
    fetch('/getSensorStatus')
        .then(response => response.json())
        .then(slotStatus => {
            const slotStatusDiv = document.getElementById('slotStatus');
            let statusHTML = '';

            // Iterate through each slot status and create HTML content
            slotStatus.forEach((status, index) => {
                statusHTML += `<p>Slot ${index + 1}: ${status ? 'Occupied' : 'Free'}</p>`;
            });

            // Update the slot status display
            slotStatusDiv.innerHTML = statusHTML;
        })
        .catch(error => console.error('Error fetching slot status:', error));
}

/**
 * Loads the admin information page, displaying the status of all slots.
 */
function loadAdminInfo() {
    const adminInfo = document.getElementById('adminInfo');
    adminInfo.innerHTML = '';  // Clear the current content

    // Fetch the updated reservations object from localStorage
    let updatedReservations = JSON.parse(localStorage.getItem('reservations')) || { 1: [], 2: [], 3: [] };

    console.log("Loading admin info, current reservations:", updatedReservations);
    console.log("Loading admin info, current reservations:", reservations);

    // Iterate through each slot and update the display based on reservation status
    for (let slotId = 1; slotId <= 3; slotId++) {
        if (updatedReservations[slotId] && updatedReservations[slotId].length > 0) {
            const userInfo = updatedReservations[slotId][0];
            adminInfo.innerHTML += `<p><strong>Slot ${slotId}</strong><br>Name: ${userInfo.fullName}<br>Car Number: ${userInfo.carNumber}<br>Email: ${userInfo.email}</p>`;
            console.log(`Slot ${slotId}: Occupied`);
        } else {
            adminInfo.innerHTML += `<p><strong>Slot ${slotId}</strong> is free.</p>`;
            console.log(`Slot ${slotId}: Free`);
        }
    }
    // Update the reservations object with the latest data
    reservations = updatedReservations;
}

// Periodically check for free slots and update the admin info and sensor status
setInterval(() => {
    fetch('/freeSlotCheck')
        .then(response => response.json())
        .then(data => {
            // If there is a slot to be freed, handle it
            if (data.slotId) {
                handleFreeSlotRequestFromArduino(data.slotId);
            }
            // Update the sensor status and admin info display
            updateSensorStatus();
            loadAdminInfo();
        })
        .catch(error => console.error('Error:', error));
}, 2000);

// Initial load of the admin info
loadAdminInfo();

/**
 * Handles the freeing of a slot triggered by the Arduino.
 * @param {number} slotId - The ID of the slot to be freed.
 */
function handleFreeSlotRequestFromArduino(slotId) {
    if (reservations[slotId].length > 0) {
        // Extract the fullName and email before freeing the slot
        const { fullName, email } = reservations[slotId][0];

        // Clear the reservation for the specified slot
        reservations[slotId] = [];
        localStorage.setItem('reservations', JSON.stringify(reservations));

        // Send the freed slot's information to the webhook
        console.log({ fullName, email });
        sendFreeSlotToWebhook({ fullName, email });

        // Update the admin info on the page
        loadAdminInfo();

        alert(`Slot ${slotId} has been freed by the ESP32.`);
    }
}

/**
 * Sends information about the freed slot to a webhook.
 * @param {Object} data - The data containing the full name and email of the freed slot's reservation.
 */
function sendFreeSlotToWebhook(data) {
    fetch('https://hook.eu2.make.com/4c3kuk26trxfw6wzye2m14xipj8y3wm1', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(data),
    })
    .then(response => response.json())
    .then(data => {
        console.log('Success:', data);
    })
    .catch((error) => {
        console.error('Error:', error);
    });
}
