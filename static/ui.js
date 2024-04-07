function getCookie(name) {
	for (const cookie of document.cookie.split("; ")) {
		const [key, value] = cookie.split("=", 2);
		if (key == name) return value;
	}
}

const me = getCookie("kakeibo_user");
let you;
switch (me) {
	case 'riku': you = 'anju'; break;
	case 'anju': you = 'riku'; break;
	default: document.location = 'welcome.html';
}

class LoadingState {
	#counter = 0;

	constructor(button) {
		this.button = button;
	}

	increment() {
		this.#counter += 1;
		this.button.classList.add("loading");
	}

	decrement() {
		this.#counter -= 1;
		if (this.#counter == 0)
			this.button.classList.remove("loading");
	}
}

const uploadForm = document.forms.upload;
const pictureSelector = uploadForm.elements.picture;
pictureSelector.onchange = () => { uploadForm.requestSubmit(); };

selectPictureButton.onclick = () => {
	pictureSelector.removeAttribute("capture");
	pictureSelector.showPicker();
};

if (pictureSelector.capture) // Supporté par le navigateur.
	takePictureButton.style.display = "inline";

takePictureButton.onclick = () => {
	pictureSelector.setAttribute("capture", "environment");
	pictureSelector.showPicker();
};

const uploadLoadingState = new LoadingState(selectPictureButton);
const receiptQueue = [];

uploadForm.onsubmit = (event) => {
	event.preventDefault();
	uploadLoadingState.increment();

	fetch("api/upload", {
		method: "POST",
		body: new FormData(uploadForm),
	}).then((response) => {
		if (response.ok)
			return response.json();
		else
			throw new Error("HTTP " + response.status);
	}).then((json) => {
		for (const receipt of json.receipts)
			receiptQueue.push(receipt);
		if (!amountField.value)
			popReceipt();
	}).catch((error) => {
		alert(error.message);
	}).finally(() => {
		uploadLoadingState.decrement();
	})
};

function popReceipt() {
	const receipt = receiptQueue.shift();
	if (!receipt)
		return false;

	dateField.value = receipt.date;
	amountField.value = receipt.total;
	return true;
}

function today() {
	return new Date().toISOString().split("T")[0];
}

const entryForm = document.forms["entry"];
const dateField = entryForm.elements["date"];
const amountField = entryForm.elements["amount"];
const remarkField = entryForm.elements["remark"];

entryForm.onreset = () => {
	dateField.defaultValue = today();
	amountField.focus();
};
entryForm.reset();

let lastFocus = Date.now();
document.onfocus = () => {
	const now = Date.now();
	if (now - lastFocus > 3600 && !amountField.value && !remarkField.value)
		entryForm.reset();
	lastFocus = now;
};

/** Bâtit le JSON d’entrée à envoyer à l’API */
function buildEntryData() {
	const data = Object.fromEntries(new FormData(entryForm));
	const amount = Number(data.amount);
	delete data.amount;
	const selectedCategory = entryForm.querySelector("label:has(input[name=category]:checked)");
	switch (selectedCategory.className) {
		case 'expense':
			data[me] = -amount;
			data[you] = null;
			break;
		case 'income':
			data[me] = amount;
			data[you] = null;
			break;
		case 'transfer':
			data[me] = -amount;
			data[you] = amount;
			break;
		default:
			alert("不正カテゴリー");
			throw("Bad category");
	}
	return data;
}

entryForm.onsubmit = () => {
	event.preventDefault();
	const data = buildEntryData();
	new Entry(data).send();
	amountField.value = null;
	remarkField.value = null;
	if (!popReceipt())
		amountField.focus();
};

document.forms.bill.onsubmit = (event) => {
	remarkField.value = event.submitter.value;
	billDialog.close();
	return false;
};

openHistoryButton.onclick = () => {
	historyDialog.showModal();
	openHistoryButton.classList.remove("error");
};

const historyLoadingState = new LoadingState(openHistoryButton);
const amountFormatter = Intl.NumberFormat("ja-JP", { signDisplay: "exceptZero" });

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
		amountCell.innerText = amountFormatter.format(data[me]) + "円";
		amountCell.title = data.category;
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
		historyLoadingState.increment();

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
			alert(error.message);
			this.#onError();
		}).finally(() => {
			historyLoadingState.decrement();
		})
	}

	#onSuccess(id) {
		this.data.id = id;
		this.#statusCell.innerText = "送信済";
		this.#statusCell.className = "success";

		this.#resendButton?.remove()

		this.#withdrawButton = document.createElement("button");
		this.#withdrawButton.innerText = "取消";
		this.#withdrawButton.onclick = () => {
			this.#withdrawButton.disabled = true;
			this.withdraw();
		};
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
		this.#resendButton.onclick = () => {
			this.#resendButton.disabled = true;
			this.send();
		};
		this.#actionsCell.appendChild(this.#resendButton);
	}

	withdraw() {
		this.#statusCell.innerText = "取消中";
		this.#statusCell.className = "loading";
		historyLoadingState.increment();

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
			alert(error.message);
			this.#onWithdrawalError();
		}).finally(() => {
			historyLoadingState.decrement();
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
