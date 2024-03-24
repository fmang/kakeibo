const pictureSelector = document.getElementById("picture-selector");
pictureSelector.addEventListener("change", (event) => { event.target.form.submit(); });

const selectPictureButton = document.getElementById("select-picture-button");
selectPictureButton.addEventListener("click", (event) => {
	pictureSelector.removeAttribute("capture");
	pictureSelector.showPicker();
});

const takePictureButton = document.getElementById("take-picture-button");
if (pictureSelector.capture == undefined) // Non supporté par le navigateur.
	takePictureButton.style.display = "none";
takePictureButton.addEventListener("click", (event) => {
	pictureSelector.setAttribute("capture", "environment");
	pictureSelector.showPicker();
});

function today() {
	return new Date().toISOString().split("T")[0];
}

const entryForm = document.forms["entry"];
const dateField = entryForm.elements["date"];
const amountField = entryForm.elements["amount"];
const remarkField = entryForm.elements["remark"];

entryForm.addEventListener("reset", (event) => {
	dateField.defaultValue = today();
	amountField.focus();
});
entryForm.reset();

let lastFocus = Date.now();
document.addEventListener("focus", (event) => {
	let now = Date.now();
	if (now - lastFocus > 3600 && !amountField.value && !remarkField.value)
		entryForm.reset();
	lastFocus = now;
});

const setTodayButton = document.getElementById("set-today-button");
setTodayButton.addEventListener("click", (event) => {
	dateField.value = today();
});

const resetButton = document.getElementById("reset-button");
resetButton.addEventListener("click", (event) => {
	event.target.closest("form").reset();
});

entryForm.addEventListener("submit", (event) => {
	event.preventDefault();
	const data = Object.fromEntries(new FormData(entryForm));
	data.id ||= null
	data.amount = Number(data.amount)

	fetch("api/send", {
		method: "POST",
		headers: { "Content-Type": "application/json" },
		body: JSON.stringify(data),
	}).then((response) => {
		return response.json();
	}).then((json) => {
		console.log(json);
	});

	entryForm.reset();
	entryForm.elements["category"].value = data.category;
});

const sendButton = document.getElementById("send-button");
sendButton.addEventListener("click", (event) => {
	entryForm.requestSubmit();
});

const billDialog = document.getElementById("bill-dialog");
const billForm = document.forms.bill;
billForm.addEventListener("submit", (event) => {
	event.preventDefault();
	remarkField.value = event.submitter.value;
	billDialog.close();
});

const billCategory = document.getElementById("bill-category");
billCategory.addEventListener("change", (event) => {
	billDialog.showModal();
});

const closeBillDialogButton = document.getElementById("close-bill-dialog-button");
closeBillDialogButton.addEventListener("click", (event) => {
	billDialog.close();
});
