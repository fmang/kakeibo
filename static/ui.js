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

entryForm.addEventListener("submit", (event) => {
	event.preventDefault();
	const data = Object.fromEntries(new FormData(entryForm));
	data.id ||= null
	data.amount = Number(data.amount)
	new Entry(data).send();
	entryForm.reset();
	entryForm.elements["category"].value = data.category;
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

const historyDialog = document.getElementById("history-dialog");
const openHistoryButton = document.getElementById("open-history-button");
openHistoryButton.addEventListener("click", (event) => {
	historyDialog.showModal();
	openHistoryButton.classList.remove("error");
});

const historyTable = document.getElementById("history-table");

class Entry {
	#row;
	#statusCell;
	#actionsCell;
	#resendButton;
	#withdrawButton;

	constructor(data) {
		this.data = data;

		const dateCell = document.createElement("td");
		dateCell.className = "numeric";
		dateCell.innerText = data.date.replace(/^\d+-0?(\d+)-0?(\d+)$/, "$1月$2日");
		const amountCell = document.createElement("td");
		amountCell.className = "numeric";
		amountCell.innerText = data.amount + "円";
		this.#statusCell = document.createElement("td");
		this.#actionsCell = document.createElement("td");
		this.#actionsCell.className = "actions";

		this.#row = document.createElement("tr");
		this.#row.appendChild(dateCell);
		this.#row.appendChild(amountCell);
		this.#row.appendChild(this.#statusCell);
		this.#row.appendChild(this.#actionsCell);
		historyTable.appendChild(this.#row);
	}

	send() {
		this.#statusCell.innerText = "送信中";
		this.#statusCell.className = "loading";
		openHistoryButton.classList.add("loading");

		fetch("api/send", {
			method: "POST",
			headers: { "Content-Type": "application/json" },
			body: JSON.stringify(this.data),
		}).then((response) => {
			if (response.ok)
				return response.json();
			else
				throw new Error("HTTP " + response.status);
		}).then((json) => {
			this.#onSuccess(json['id']);
		}).catch((error) => {
			console.log(error.message);
			this.#onError();
		}).finally(() => {
			openHistoryButton.classList.remove("loading");
		})
	}

	#onSuccess(id) {
		this.data.id = id;
		this.#statusCell.innerText = "送信済";
		this.#statusCell.className = "success";

		this.#resendButton?.remove()

		this.#withdrawButton = document.createElement("button");
		this.#withdrawButton.innerText = "取消";
		this.#withdrawButton.addEventListener("click", (event) => {
			this.#withdrawButton.disabled = true;
			this.withdraw();
		});
		this.#actionsCell.appendChild(this.#withdrawButton);
	}

	#onError() {
		this.#statusCell.innerText = "失敗";
		this.#statusCell.className = "error";
		if (!historyDialog.open)
			openHistoryButton.classList.add("error");

		if (this.#resendButton) {
			this.#resendButton.disabled = false;
			return;
		}

		this.#resendButton = document.createElement("button");
		this.#resendButton.innerText = "再試行";
		this.#resendButton.addEventListener("click", (event) => {
			this.#resendButton.disabled = true;
			this.send();
		});
		this.#actionsCell.appendChild(this.#resendButton);
	}

	withdraw() {
		this.#statusCell.innerText = "取消中";
		this.#statusCell.className = "loading";
		openHistoryButton.classList.add("loading");

		fetch("api/withdraw", {
			method: "POST",
			headers: { "Content-Type": "application/json" },
			body: JSON.stringify({ id: this.data.id }),
		}).then((response) => {
			if (response.ok)
				return response.json();
			else
				throw new Error("HTTP " + response.status);
		}).then((json) => {
			this.#onWithdrawalSuccess();
		}).catch((error) => {
			console.log(error.message);
			this.#onWithdrawalError();
		}).finally(() => {
			openHistoryButton.classList.remove("loading");
		})
	}

	#onWithdrawalSuccess() {
		this.#statusCell.innerText = "取消済";
		this.#statusCell.className = "success";
		this.#row.classList.add("withdrawn");
	}

	#onWithdrawalError() {
		this.#statusCell.innerText = "取消失敗";
		this.#statusCell.className = "error";
		if (!historyDialog.open)
			openHistoryButton.classList.add("error");
		this.#withdrawButton.disabled = false;
	}
}
